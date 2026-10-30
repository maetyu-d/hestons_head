#include "Theme.h"
#include "ReagentWorks.h"
#include "Interface.h"
#include "Power.h"
#include "WaterWorks.h"
#include "PowerArt.h"
#include "RouteArt.h"
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

}
void Interface::drawEnergyRoutes(bool overlay){
 const float tile=38*factoryZoom;if(tile<8)return;
 const double clock=factory.tick+(factory.running?factoryAccumulator/.55:0);
 using E=EnergyMedium;using Key=std::pair<Position,E>;
 std::set<E> focused;bool equipmentFocus=false;
 if(energyLinkSelected){focused.insert(selectedEnergyMedium);equipmentFocus=true;}
 else if(auto m=factory.at(factorySelected);m&&isEnergyMachine(m->kind)){for(auto port:energyPorts(*m))focused.insert(port.medium);equipmentFocus=true;}
 if(!equipmentFocus)focused={E::AC,E::DC,E::Shaft,E::Steam,E::Heat,E::RawWater,E::FilteredWater,E::PurifiedWater,E::CleanWater,E::Greywater,E::Concentrate};
 auto visible=[&](E medium){return energyRoutesAll||focused.count(medium);};
 auto terminal=[&](Position p,E medium,int side)->std::optional<Vector2>{
  auto m=factory.at(p);if(!m||!isEnergyMachine(m->kind))return {};
  Machine unrotated;unrotated.kind=m->kind;auto ports=energyPorts(unrotated);
  for(size_t n=0;n<ports.size();n++){auto port=ports[n];if(port.medium!=medium||(port.direction+m->dir)%4!=side)continue;float shift=0;for(size_t j=0;j<ports.size();j++)if(j!=n&&ports[j].direction==port.direction)shift=j<n?.07f:-.07f;
   float u=port.direction==0?.96f:port.direction==2?.04f:.5f+shift,v=port.direction==1?.96f:port.direction==3?.04f:.5f+shift;
   if(lineFamily(medium)==LineFamily::Electricity&&(isReagentMachine(m->kind)||isWaterMachine(m->kind))){if(port.direction%2==0)v=.82f;else u=.82f;}
   for(int turn=0;turn<m->dir;turn++){float old=u;u=1-v;v=old;}auto r=factoryTile(p);float pad=std::max(1.f,tile*.065f);return Vector2{r.x+pad+u*(tile-2*pad),r.y+pad+v*(tile-2*pad)};
  }return {};
 };
 struct Rail {Vector2 point;const PowerLink* link;int degree=0;};
 std::map<Key,Rail> rails;
 for(const auto& [p,links]:factory.energyLinks)for(const auto& link:links)if(visible(link.medium)){auto r=factoryTile(p);rails.emplace(Key{p,link.medium},Rail{{r.x+tile*.5f,r.y+tile*.5f},&link,0});}
 // Align whole straight runs with their real terminal, rather than inserting a
 // tiny dogleg at each machine. Horizontal and vertical rails solve independently.
 const std::array<float,EnergyCount> lane={0,-.08f,0,.06f,-.06f,.10f,-.10f,.08f,-.08f,.12f,.14f,-.14f,.18f,.20f,.22f,.24f,-.18f,-.22f};
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
   if(port){bool source=false;if(auto device=factory.at(q))for(auto pin:energyPorts(*device))if(pin.medium==medium&&pin.direction==(d+2)%4&&pin.output)source=true;rail.degree++;edges.push_back({pathBetween(rail.point,*port,d,p),medium,rail.link,source,true,source||rail.link->dir==d?rail.link->flow:0});}
   else if(neighbor!=rails.end()&&(neighbor->second.link->ports&(1<<((d+2)%4)))){rail.degree++;if(q<p)continue;const PowerLink* flow=rail.link;bool reverse=false;double rate=rail.link->dir==d?rail.link->flow:0;if(neighbor->second.link->flow>0&&neighbor->second.link->dir==(d+2)%4){flow=neighbor->second.link;reverse=true;rate=flow->flow;}edges.push_back({pathBetween(rail.point,neighbor->second.point,d,p),medium,flow,reverse,false,rate});}
   else if(medium==E::AC){auto consumer=factory.at(q);if(consumer&&!isEnergyMachine(consumer->kind)&&machineSpec(consumer->kind).power>0&&!factory.energyLink(q,medium)){auto r=factoryTile(q);rail.degree++;edges.push_back({pathBetween(rail.point,{r.x+tile*.5f,r.y+tile*.5f},d,p),medium,rail.link,false,true,rail.link->dir==d?rail.link->flow:0});}}
  }
 }
 for(const auto& [p,m]:factory.machines)if(isEnergyMachine(m.kind))for(auto port:energyPorts(m)){if(!visible(port.medium)||(port.direction!=0&&port.direction!=1))continue;Position q=nextPosition(p,port.direction);auto a=terminal(p,port.medium,port.direction),b=terminal(q,port.medium,(port.direction+2)%4);if(a&&b){auto other=factory.at(q);if(other&&isContactService(port.medium)&&powerContactSupports(m.kind,other->kind,port.medium))continue;double flow=factory.powerContactFlow(p,q,port.medium);edges.push_back({pathBetween(*a,*b,port.direction,p),port.medium,nullptr,std::abs(flow)>.001?flow<0:!port.output,true,std::abs(flow)});}}
 // Water is heavy, round-coupled pipework. Electrical trays remain thin,
 // square and insulated. Draw the cables last, with an air gap at crossings.
 if(!overlay)for(int layer=0;layer<2;layer++)for(const auto& edge:edges){
  if((lineFamily(edge.medium)==LineFamily::Electricity)!=(layer==1))continue;
  drawInfrastructureRoute(edge.path,edge.medium,tile,!edge.link||edge.link->enabled,edge.link&&edge.link->tripped,equipmentFocus);
 }
 if(!overlay)for(const auto& [key,rail]:rails)if(rail.degree>2)drawInfrastructureJunction(rail.point,key.second,tile);
 for(const auto& edge:edges){
  if(edge.path.size()<2)continue;
  if(!overlay&&edge.boot){auto end=edge.path.back(),before=edge.path[edge.path.size()-2];drawInfrastructureFitting(end,{end.x-before.x,end.y-before.y},edge.medium,tile,!edge.link||edge.link->enabled);}
  else if(!overlay&&tile>=32){auto mid=wirePoint(edge.path,.5f),ahead=wirePoint(edge.path,.51f);drawInfrastructureFitting(mid,{ahead.x-mid.x,ahead.y-mid.y},edge.medium,tile,!edge.link||edge.link->enabled);}
  if(overlay&&floorFlow&&(edge.flow>.001||edge.boot))drawInfrastructureDirection(edge.path,edge.medium,tile,edge.reverse,edge.flow>.001);
  if(overlay&&edge.link&&(!edge.link->enabled||edge.link->tripped))drawInfrastructureDirection(edge.path,edge.medium,tile,false,false,true);
  if(overlay&&floorFlow&&(!edge.link||(edge.link->enabled&&!edge.link->tripped))&&edge.flow>.001){float phase=float(std::fmod(clock*(.6+std::min(1.,edge.flow/(edge.link?edge.link->rating:40.))),1.));drawInfrastructureFlow(edge.path,edge.medium,tile,phase,edge.reverse);}
 }
 // Bench electricity is a small square plug below the material path. Working
 // water has round flanges; unused possible services do not become phantom pipes.
 for(const auto& [p,m]:factory.machines)if(isEnergyMachine(m.kind))for(int d:{0,1}){
  Position q=nextPosition(p,d);auto other=factory.at(q);if(!other||!isEnergyMachine(other->kind))continue;
  std::vector<E> pins;for(size_t n=0;n<EnergyCount;n++){
   E medium=E(n);if(!powerContactSupports(m.kind,other->kind,medium)||!visible(medium))continue;
   bool needed=medium==E::AC||std::abs(factory.powerContactFlow(p,q,medium))>.001;
   for(const auto* device:std::array<const Machine*,2>{&m,other})for(auto port:energyPorts(*device))if(port.medium==medium)needed=true;
   if(needed)pins.push_back(medium);
  }
  if(pins.empty())continue;
  auto r=factoryTile(p);Vector2 edge{r.x+(d==0?tile:tile*.5f),r.y+(d==1?tile:tile*.5f)};
  bool live=m.enabled&&!m.power.tripped&&other->enabled&&!other->power.tripped;
  bool laboratory=isReagentMachine(m.kind)||isReagentMachine(other->kind)||isWaterMachine(m.kind)||isWaterMachine(other->kind);
  float span=std::clamp(tile*.18f,3.f,15.f);
  for(size_t n=0;n<pins.size();n++){
   E medium=pins[n];float offset=laboratory&&lineFamily(medium)==LineFamily::Electricity?tile*.30f:(float(n)-(pins.size()-1)*.5f)*std::clamp(tile*.045f,1.7f,4.f);
   Vector2 center{edge.x+(d==1?offset:0),edge.y+(d==0?offset:0)};
   Vector2 a{center.x-(d==0?span*.5f:0),center.y-(d==1?span*.5f:0)},b{center.x+(d==0?span*.5f:0),center.y+(d==1?span*.5f:0)};
   if(!overlay){drawInfrastructureRoute({a,b},medium,tile,live,false,equipmentFocus);drawInfrastructureFitting(center,{b.x-a.x,b.y-a.y},medium,tile,live);}
   double flow=live?factory.powerContactFlow(p,q,medium):0;
   if(overlay&&floorFlow&&std::abs(flow)>.001){drawInfrastructureDirection({a,b},medium,tile,flow<0,true);float phase=float(std::fmod(clock*1.4,1.));drawInfrastructureFlow({a,b},medium,tile,phase,flow<0);}
  }
 }
 if(overlay)for(const auto& [key,rail]:rails)if(rail.link->tripped||rail.link->heat>.65){DrawRectangleRec({rail.point.x-2,rail.point.y-2,4,4},Color{220,94,63,255});}
 if(overlay&&floorFlow&&tile>=30)for(const auto& [p,m]:factory.machines)if(p==factorySelected&&isEnergyMachine(m.kind)){
  auto r=factoryTile(p);if(!CheckCollisionRecs(r,factoryView))continue;
  for(auto port:energyPorts(m)){if(!visible(port.medium)||port.medium==E::Instrument)continue;auto q=nextPosition(p,port.direction);if(factory.energyLink(q,port.medium)||terminal(q,port.medium,(port.direction+2)%4))continue;auto at=terminal(p,port.medium,port.direction);if(!at)continue;auto d=directions[port.direction];float sign=port.output?1.f:-1.f,span=std::clamp(tile*.12f,4.f,9.f);Vector2 a{at->x-d[0]*span*sign,at->y-d[1]*span*sign},b{at->x+d[0]*span*sign,at->y+d[1]*span*sign};
   drawInfrastructureDirection({a,b},port.medium,tile,false,false,!m.enabled||m.power.tripped);
  }
 }
 if(overlay&&equipmentFocus&&!energyRoutesAll){std::string label=focused.size()==1?upper(energyName(*focused.begin())):"SELECTED EQUIPMENT SERVICES";text(label+" / service overlay",factoryView.x+10,factoryView.y+30,8.5f,Color{77,99,131,255},true);}
}
void Interface::drawPowerInspector(Rectangle r){
 const Color ink{22,41,75,255},muted{77,99,131,255},line{196,205,217,255},orange{240,43,43,255},green{13,162,65,255};DrawRectangleRec(r,WHITE);DrawRectangleLinesEx(r,1,line);float x=r.x+12,y=r.y+12,w=r.width-24;auto m=factory.at(factorySelected);auto link=energyLinkSelected?factory.energyLink(factorySelected,selectedEnergyMedium):nullptr;
 text(link?upper(energyName(link->medium)):m?upper(machineSpec(m->kind).name):"POWER ENGINEERING",x,y,11,ink,true);y+=23;
 text("TILE "+positionName(factorySelected),x,y,9.5f,muted);y+=20;
 Rectangle art{x,y,w,160};DrawRectangleRounded(art,.08f,5,Color{255,255,255,255});MachineAppearance look;look.kind=link?energyConduit(link->medium):m?m->kind:MachineKind::PowerCable;look.direction=m?m->dir:0;look.active=factory.running&&(link?link->enabled&&!link->tripped&&link->flow>.001:m&&m->enabled&&!m->power.tripped&&(m->power.flow>.001||m->power.generated>.001||m->power.rpm>20||m->power.lab.flux>.01));look.closed=link?link->tripped:m&&!m->enabled;look.time=factory.tick+(factory.running?factoryAccumulator/.55:0);if(m){look.labFlux=m->power.lab.flux;look.labVacuum=m->power.lab.vacuum;look.labStability=m->power.lab.stability;look.labCooling=m->power.lab.cooling;look.labArmed=m->power.lab.armed;double stored=0;for(auto v:m->power.store)stored+=v;look.progress=stored/std::max(1.,m->power.capacity);if(isReagentMachine(m->kind)){look.active=factory.running&&m->enabled&&m->power.delivered>.001;look.reagentFill=m->item?std::min(1.,analyse(m->item->food).mass/100):m->reagentProcess.minerals/500;look.reagentPurity=m->item?reagentPurity(*m->item):1;look.reagentKey=m->item?m->item->reagent.key:m->ingredient;}if(isWaterMachine(m->kind)){auto q=waterSample(*m);if(q.volume>1e-8){look.waterDirt=q.solids/q.volume;look.waterSalt=q.salt/q.volume;look.waterBio=q.bio/q.volume;}}}else if(link)look.progress=link->flow/link->rating;
 drawMachineHousing({art.x+5,art.y+5,145,145},look);drawMachineMechanism({art.x+5,art.y+5,145,145},look);float tx=x+154,tw=w-160;if(tw>30){text(link?(lineFamily(link->medium)==LineFamily::Water?"WATER PIPE":lineFamily(link->medium)==LineFamily::Electricity?"ELECTRICITY CABLE":"SERVICE CONDUIT"):m&&isNuclearEquipment(m->kind)?"LAB APPARATUS":m&&isWaterMachine(m->kind)?"LIVE WATER":m&&isReagentMachine(m->kind)?"LIVE REAGENT":"LIVE POWER",tx,y+13,8.5f,muted,true);text(fit(link?(link->tripped?"TRIPPED":link->enabled?(link->flow>.001?"FLOWING":"IDLE / NO TRANSFER"):"ISOLATED"):m?m->status:"",tw,10,true),tx,y+34,10,link&&link->tripped?orange:green,true);text(m&&isReagentMachine(m->kind)?(m->item?roundTo(analyse(m->item->food).mass,1)+" g / LOT":"NO LOT"):link?roundTo(link->flow,1)+(lineFamily(link->medium)==LineFamily::Water?" L/TICK":" / TICK"):m?roundTo(m->kind==MachineKind::CuriosityPile||m->kind==MachineKind::StarChamber?m->power.flow:isWaterMachine(m->kind)?m->power.flow:m->power.generated,1)+(isWaterMachine(m->kind)?" L/TICK":m->kind==MachineKind::CuriosityPile||m->kind==MachineKind::StarChamber?" HEAT":" GEN"):"",tx,y+62,11,ink,true);text(m&&isReagentMachine(m->kind)?(m->item?roundTo(reagentPurity(*m->item)*100,1)+"% PURE":"AC "+roundTo(m->power.delivered,2)):link?"HEAT "+roundTo(link->heat*100,0)+"%":m?(m->kind==MachineKind::WaterOutlet?roundTo(m->waterProcess.delivered,1)+" L SUPPLIED":roundTo(m->power.delivered,1)+(isWaterMachine(m->kind)?" L OUT":" OUT")):"",tx,y+83,9,muted);}
 y+=170;
 inspectorBody={x,y,w,r.y+r.height-y-14};inspectorScroll=std::clamp(inspectorScroll,0.f,std::max(0.f,inspectorHeight-inspectorBody.height));beginClip(inspectorBody);y-=inspectorScroll;float start=y;
 button({x,y,w-4,27},energyRoutesAll?"Routes / all services":"Routes / selected services",[this]{energyRoutesAll=!energyRoutesAll;},energyRoutesAll,true,"Show all services, or isolate only the services connected to this equipment",{},10);y+=35;
 auto pair=[&](const std::string& label,const std::string& less,const std::string& more){text(label,x,y+7,10.5f,ink,true);button({x+w-58,y,24,27},"-",[this,less]{if(energyLinkSelected)factory.configureEnergyLink(factorySelected,selectedEnergyMedium,less);else factory.configure(factorySelected,less);},false,true,"Adjust this physical setting",{},12);button({x+w-29,y,24,27},"+",[this,more]{if(energyLinkSelected)factory.configureEnergyLink(factorySelected,selectedEnergyMedium,more);else factory.configure(factorySelected,more);},false,true,"Adjust this physical setting",{},12);y+=34;};
 if(link){button({x,y,w-4,28},"Layer / "+energyName(link->medium),[this]{auto i=factory.energyLinks.find(factorySelected);if(i==factory.energyLinks.end())return;for(size_t n=0;n<i->second.size();n++)if(i->second[n].medium==selectedEnergyMedium){selectedEnergyMedium=i->second[(n+1)%i->second.size()].medium;break;}},false,true,"Cycle conduits sharing this tile",{},10);y+=36;
  pair("Rating "+roundTo(link->rating,0),"rating-","rating+");float bw=(w-10)/2;button({x,y,bw,28},link->enabled?"Isolate":"Reconnect",[this]{factory.configureEnergyLink(factorySelected,selectedEnergyMedium,"enabled");},false,true,"Open or close this route",{},10);button({x+bw+5,y,bw,28},"Reset trip",[this]{factory.configureEnergyLink(factorySelected,selectedEnergyMedium,"reset");},false,link->tripped,"Reset after the conductor has cooled",{},10);y+=36;button({x,y,w-4,28},"Reclaim this conduit",[this]{factory.removeEnergyLink(factorySelected,selectedEnergyMedium);energyLinkSelected=false;},false,true,"Remove only this energy layer; floor machines remain in place",orange,10);y+=40;
  y+=paragraph("Different media cross without joining. Matching conduits join at visible junctions. Electrical routes have a finite rating and accumulate overload heat; a tripped conductor carries no power.",x,y,w,10.5f,muted)+12;
 }else if(m){
  if(isReagentMachine(m->kind)){
   auto& chemical=m->reagentProcess;
   y+=paragraph(m->status,x,y,w,11,green,true)+12;
   auto recipes=reagentRecipes(m->kind);
   if(!recipes.empty()){button({x,y,w-4,29},"Make / "+ingredient(m->ingredient).name,[this]{factory.configure(factorySelected,"reagent-recipe");},false,!m->item,"Cycle the reaction after the current lot has left",{},10);y+=38;pair("Dwell "+std::to_string(m->duration)+" ticks","dwell-","dwell+");pair("Target "+roundTo(m->temperature,0)+" C","temp-","temp+");}
   if(m->kind==MachineKind::ReagentEvaporator)pair("Target "+roundTo(m->temperature,0)+" C","temp-","temp+");
   if(m->kind==MachineKind::ReagentSeparator)pair("Selectivity "+roundTo(m->shear/2*100,0)+"%","shear-","shear+");
   if(m->kind==MachineKind::MineralHopper){text("RAW MINERAL FEED / "+roundTo(chemical.minerals,1)+" g",x,y,10,ink,true);y+=23;button({x,y,w-4,29},"Import raw minerals +500 g",[this]{factory.configure(factorySelected,"reagent-minerals");},false,true,"20 credits in Economy; this is raw feed, not finished reagents",{},10);y+=38;pair("Feed dose "+roundTo(m->dose,1)+" g","dose-","dose+");}
   if(m->kind==MachineKind::CultureVat){text("STARTER / "+roundTo(chemical.starter,2)+" g",x,y,10,ink,true);y+=23;button({x,y,w-4,29},"Load starter culture +10 g",[this]{factory.configure(factorySelected,"reagent-starter");},false,true,"Finite bootstrap culture; costs 8 credits in Economy",{},10);y+=38;button({x,y,w-4,29},"Use own culture / 1 g",[this]{factory.configure(factorySelected,"reagent-inoculate");},false,true,"Moves your propagated culture from named stock into this vat",{},10);y+=38;}
   const Batch* sample=m->item?&*m->item:nullptr;
   if(sample&&!sample->reagent.key.empty()){
    text(upper(ingredient(sample->reagent.key).name)+" / LOT #"+std::to_string(sample->id),x,y,10,ink,true);y+=23;
    text("Active "+roundTo(sample->reagent.active,3)+" g / impurity "+roundTo(sample->reagent.impurity,3)+" g",x,y,10,muted);y+=21;
    text("PURITY "+roundTo(reagentPurity(*sample)*100,1)+"% / CONC "+roundTo(reagentConcentration(*sample)*100,1)+"%",x,y,10,ink,true);y+=21;
    text(sample->reagent.assayed?"ASSAY / certified actual lot":"ASSAY / measurement required",x,y,10,sample->reagent.assayed?green:orange,true);y+=26;
   }
   if(m->kind==MachineKind::ReagentAnalyzer){text("LAST ASSAY / LOT #"+std::to_string(m->sensorBatch),x,y,10,ink,true);y+=23;text("Purity "+roundTo(chemical.samplePurity*100,1)+"% / concentration "+roundTo(chemical.sampleConcentration*100,1)+"%",x,y,9.5f,muted);y+=25;}
   if(m->kind==MachineKind::ReagentTank){button({x,y,w-4,29},m->forward?"After assay / forward":"Collection / hold",[this]{factory.configure(factorySelected,"reagent-forward");},m->forward,true,"Hold a lot for examination, or forward it once measured",{},10);y+=38;}
   if(m->kind==MachineKind::ReagentIntake){y+=paragraph("Accepts only assayed lots with at least 85% dry purity and 35% concentration. The named active ingredient enters stock; carrier water returns to treatment.",x,y,w,10.5f,muted)+10;button({x,y,w-4,29},"Send held lot to side outlet",[this]{factory.configure(factorySelected,"reagent-reject");},false,bool(m->item)&&!m->sideOutput,"Reject an off-spec lot for recovery or further purification",{},10);y+=38;}
   if(m->kind==MachineKind::ReagentIntake||m->kind==MachineKind::ReagentRecovery){text("HELD SOLIDS / "+roundTo(chemical.waste,1)+" / "+roundTo(m->power.capacity,0)+" g",x,y,10,ink,true);y+=23;button({x,y,w-4,29},"Haul held solids",[this]{factory.configure(factorySelected,"reagent-haul");},false,chemical.waste>0,"Removes recorded solids; Economy costs .01 credits/g",{},10);y+=38;}
   float half=(w-10)/2;button({x,y,half,28},m->enabled?"Disable":"Enable",[this]{factory.configure(factorySelected,"enabled");},false,true,"Pause this apparatus",{},10);button({x+half+5,y,half,28},"Rotate ports",[this]{factory.rotate(factorySelected);},false,true,"Rotate the material and assay direction",{},10);y+=38;
   pair("Throttle "+roundTo(m->power.throttle*100,0)+"%","power-throttle-","power-throttle+");
   if(m->sideOutput){text("SIDE / "+roundTo(analyse(m->sideOutput->food).mass,2)+" g",x,y,10,orange,true);y+=23;}
   static const std::array<const char*,4> side={"E","S","W","N"};
   text("MATERIAL PORTS / glass or conveyor",x,y,10,ink,true);y+=22;
   if(m->kind!=MachineKind::MineralHopper&&m->kind!=MachineKind::ReagentAnalyzer){text(std::string("IN ")+side[size_t((m->dir+2)%4)]+" / raw material or lot",x,y,10,muted);y+=20;}
   text(std::string(m->kind==MachineKind::ReagentAnalyzer?"PROBE ":"OUT ")+side[size_t(m->dir)]+" / "+(m->kind==MachineKind::ReagentAnalyzer?"adjacent sample":"main product"),x,y,10,muted);y+=20;
   if(hasSideOutlet(m->kind)){text(std::string("SIDE ")+side[size_t((m->dir+1)%4)]+" / residue or condensate",x,y,10,orange);y+=20;}
   y+=paragraph(machineSpec(m->kind).description,x,y,w,10.5f,muted)+16;
   text("NAMED FACTORY REAGENTS",x,y,10,ink,true);y+=23;
   for(const auto& [key,kg]:factory.reagentworks.inventory)if(kg>1e-8){text(ingredient(key).name+" / "+roundTo(kg*1000,2)+" g",x,y,10,muted);y+=20;}
   text("Generic imports / "+roundTo(genericChemicals(factory)*1000,1)+" g",x,y,10,muted);y+=23;
   text("Lab supplied "+roundTo(factory.reagentworks.delivered,2)+" g",x,y,10,ink,true);y+=21;text("Water returned "+roundTo(factory.reagentworks.returnedWater,3)+" L",x,y,10,muted);y+=24;
   y+=paragraph("Finished reagent inventory is specific to its ingredient. Touching lab apparatus shares AC; glass tubing and conveyors carry real material. Side receivers have finite capacity.",x,y,w,10.5f,muted)+12;
   inspectorHeight=y-start;endClip();return;
  }
  if(isWaterMachine(m->kind)){
   auto q=waterSample(*m);text("WATER / "+roundTo(q.volume,2)+" L sample",x,y,10,ink,true);y+=21;
   if(q.volume>1e-8){for(auto reading:{std::pair<std::string,double>{"Suspended solids",q.solids/q.volume},{"Dissolved salts",q.salt/q.volume},{"Biological load",q.bio/q.volume}}){text(reading.first+" / "+roundTo(reading.second,3)+" per L",x,y,10,muted);y+=19;}}
   text("GAME LIMITS / solids .15 / salt 1 / bio .02",x,y,8.5f,muted,true);y+=24;
   text("FLOW "+roundTo(m->power.flow,2)+" L/tick",x,y,10,ink,true);y+=24;
   if(m->kind==MachineKind::BoreholePump){text("AQUIFER "+roundTo(factory.waterworks.aquifer,1)+" / "+roundTo(factory.waterworks.aquiferCapacity,0)+" L",x,y,10,muted,true);y+=24;}
   if(m->kind==MachineKind::RainCollector){text("RAIN "+roundTo(waterRainfall(factory.tick)*100,0)+"%",x,y,10,muted,true);y+=24;}
   if(m->kind==MachineKind::SandFilter||m->kind==MachineKind::MembraneFilter){text("Fouling "+roundTo(m->waterProcess.fouling*100,0)+"% / held solids "+roundTo(m->waterProcess.captured,2),x,y,9.5f,muted);y+=23;button({x,y,w-4,28},"Backwash / 2 L clean water",[this]{factory.configure(factorySelected,"water-rinse");},false,true,"Uses actual water and AC; sends retained solids to concentrate",{},10);y+=36;}
   if(m->kind==MachineKind::MembraneFilter)pair("Recovery "+roundTo(m->waterProcess.recovery*100,0)+"%","water-recovery-","water-recovery+");
   if(m->kind==MachineKind::UVSterilizer)pair("UV dose "+roundTo(m->waterProcess.uvDose*100,0)+"%","water-dose-","water-dose+");
   if(m->kind==MachineKind::WaterOutlet)pair("Factory target "+roundTo(m->dose,0)+" L","water-target-","water-target+");
   if(m->kind==MachineKind::WaterOutlet||m->kind==MachineKind::CleanWaterTank){button({x,y,w-4,28},"Purge held water to reject",[this]{factory.configure(factorySelected,"water-purge");},false,true,"A connected concentrate route must have room",{},10);y+=36;}
   if(m->kind==MachineKind::ConcentrateTank){button({x,y,w-4,28},"Haul concentrate away",[this]{factory.configure(factorySelected,"water-haul");},false,true,"Records actual disposed volume and loads; costs 0.02 credits/L in Economy",{},10);y+=36;}
   text("FACTORY SUPPLY / "+roundTo(factory.waterworks.delivered,1)+" L delivered",x,y,9.5f,muted,true);y+=21;text("RETURN / "+roundTo(factory.waterworks.greywater.volume,1)+" L waiting",x,y,9.5f,muted);y+=21;text("RECOVERY / "+roundTo(factory.waterworks.recovered,1)+" L reclaimed",x,y,9.5f,muted);y+=25;
  }
  if(isNuclearEquipment(m->kind)){
   const auto& lab=m->power.lab;bool chamber=m->kind==MachineKind::CuriosityPile||m->kind==MachineKind::StarChamber;
   text("EXPERIMENT / abstract lab units",x,y,9,muted,true);y+=22;
   auto meter=[&](const std::string& label,double value,Color color){text(label+" / "+roundTo(value*100,0)+"%",x,y,9.5f,ink,true);y+=16;DrawRectangleRec({x,y,w-4,4},Color{237,240,244,255});DrawRectangleRec({x,y,float((w-4)*std::clamp(value,0.,1.)),4},color);y+=14;};
   if(chamber){
    meter("Excitation",lab.flux,Color{166,126,182,255});meter("Stability",lab.stability,green);meter("Cold jacket",lab.cooling,Color{75,170,195,255});if(m->kind==MachineKind::StarChamber)meter("Vacuum",lab.vacuum,Color{155,140,181,255});
    text("SEALED ODDIUM / "+roundTo(lab.fuel,1)+" charge / "+roundTo(lab.spent,1)+" spent",x,y,9.5f,ink,true);y+=22;
    float bw=(w-10)/2;button({x,y,bw,28},lab.armed?"SCRAM":"Arm experiment",[this]{factory.configure(factorySelected,"lab-arm");},lab.armed,!m->power.tripped,"Stop excitation immediately; residual heat still needs to cool",lab.armed?orange:Color{},10);button({x+bw+5,y,bw,28},"Change cartridge",[this]{factory.configure(factorySelected,"lab-load");},false,!lab.armed&&lab.flux<.05&&m->power.temperature<=55,"Uses 0.40 kg supplied chemicals for 40 fictional cartridge charge; requires a cool disarmed bench",{},10);y+=36;
    pair("Resonance "+roundTo(lab.tuning*100,0),"lab-tune-","lab-tune+");if(m->kind==MachineKind::StarChamber)pair("Pulse interval "+std::to_string(std::max(2,m->duration)),"dwell-","dwell+");
    text("JACKET "+roundTo(m->power.temperature,1)+" C",x,y,10,muted,true);y+=21;
   }
   if(chamber){button({x,y,w-4,26},"Deliver chemicals for cartridges",[this]{factory.receiveChemicals();},false,true,"Receive chemical input for subsequent sealed cartridge changes",{},10);y+=34;}
   if(m->kind==MachineKind::QuenchBath){text("WATER INPUT / "+roundTo(factory.water,1)+" units",x,y,10,ink,true);y+=21;button({x,y,w-4,28},"Deliver water +100",[this]{factory.refillWater();},false,true,"Replenish the factory's supplied water input for the quench loop",{},10);y+=36;}
   if(m->kind==MachineKind::ExperimentConsole){meter("Measured stability",lab.stability,green);text("RECORDED RUNS / 24 powered ticks",x,y,9.5f,ink,true);y+=22;
    if(lab.trials.empty())y+=paragraph("Run the wired experiment for 24 ticks to capture the first reading. Change resonance or drive, then compare recovered DC with electrical draw.",x,y,w,10,muted)+12;
    for(auto i=lab.trials.rbegin();i!=lab.trials.rend()&&i-lab.trials.rbegin()<4;++i){text("T"+std::to_string(i->tick)+" / heat "+roundTo(i->heat,1)+" / stable "+roundTo(i->stability*100,0)+"%",x,y,9.5f,ink,true);y+=17;text("DRAW "+roundTo(i->draw,1)+" / DC "+roundTo(i->dc,1)+" / NET "+roundTo(i->dc-i->draw,1),x,y,9.5f,i->dc>=i->draw?green:orange,true);y+=17;text(fit(i->settings,w,8.5f),x,y,8.5f,muted);y+=21;}
    y+=paragraph("Net compares recovered DC with chamber, pump, bath and recorder electricity. r = resonance, d = drive, p = pulse interval. Inverter and distribution losses occur downstream.",x,y,w,9.5f,muted)+10;
    button({x,y,w-4,26},"Clear recorded runs",[this]{factory.configure(factorySelected,"lab-log-clear");},false,!lab.trials.empty(),"Clear observations; actual fuel and accumulated energy are retained",{},10);y+=34;
   }
   if(m->kind==MachineKind::Calorimeter){meter("Cold reference",lab.cooling,Color{75,170,195,255});text("HEAT IN "+roundTo(lab.heatTotal,1)+" / DC OUT "+roundTo(lab.dcTotal,1),x,y,10,ink,true);y+=22;}
  }
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
  text("BENCH CONTACTS",x,y,10,ink,true);y+=20;
  static const std::array<const char*,4> sides={"E","S","W","N"};int contacts=0;
  for(int d=0;d<4;d++){auto neighbor=factory.at(nextPosition(factorySelected,d));if(!neighbor||!isEnergyMachine(neighbor->kind))continue;contacts++;bool live=m->enabled&&!m->power.tripped&&neighbor->enabled&&!neighbor->power.tripped;text(fit(std::string(sides[size_t(d)])+" / "+machineSpec(neighbor->kind).name+(live?" / docked":" / isolated"),w,9.5f,true),x,y,9.5f,live?green:muted,true);y+=19;for(size_t n=0;n<EnergyCount;n++){auto medium=EnergyMedium(n);double transfer=factory.powerContactFlow(factorySelected,nextPosition(factorySelected,d),medium);if(std::abs(transfer)<=.001)continue;text(fit(std::string(transfer>0?"  OUT > ":"  IN < ")+energyName(medium)+" / "+roundTo(std::abs(transfer),2)+" per tick",w,9,true),x,y,9,energyColor(medium),true);y+=16;}}
  y+=paragraph(m->kind==MachineKind::SolarPanel?"Panel edges share DC with adjoining panels, batteries and inverters. Arrange a row or a block; AC export starts at the inverter. Gaps and diagonal neighbours do not connect.":contacts?"Touching chassis share separate AC, DC, return water, cooling, vacuum and control services. Heat and shaft stages dock through matching process ports. Gaps need conduits.":"Place power equipment edge to edge to dock its bench services. Use a cable from the output to the factory.",x,y,w,9.5f,muted)+12;
  text("EXTERNAL / PROCESS PORTS",x,y,10,ink,true);y+=20;static const std::array<const char*,4> names={"E","S","W","N"};for(auto port:energyPorts(*m)){Position q=nextPosition(factorySelected,port.direction);auto cable=factory.energyLink(q,port.medium);bool connected=cable&&(cable->ports&(1<<((port.direction+2)%4)));if(auto adjacent=factory.at(q);adjacent&&isEnergyMachine(adjacent->kind))for(auto other:energyPorts(*adjacent))if(other.medium==port.medium&&other.direction==(port.direction+2)%4)connected=true;bool docked=false;if(isContactService(port.medium)&&m->enabled&&!m->power.tripped)for(int d=0;d<4;d++){auto n=factory.at(nextPosition(factorySelected,d));if(n&&powerContactSupports(m->kind,n->kind,port.medium)&&n->enabled&&!n->power.tripped)docked=true;}text(fit(std::string(port.output?"OUT ":"IN ")+names[size_t(port.direction)]+" / "+energyName(port.medium)+(connected?" / linked":docked?" / via bench":" / unconnected"),w,9.5f,true),x,y,9.5f,connected||docked?energyColor(port.medium):orange,true);y+=18;}
  y+=9;text("ACTUAL STORES",x,y,10,ink,true);y+=20;for(size_t n=0;n<EnergyCount;n++){if(m->power.store[n]<=.001)continue;text(energyName(EnergyMedium(n))+" / "+roundTo(m->power.store[n],2),x,y,9.5f,energyColor(EnergyMedium(n)),true);y+=16;float fraction=float(std::clamp(m->power.store[n]/std::max(1.,m->power.capacity),0.,1.));DrawRectangleRec({x,y,w-4,4},Color{237,240,244,255});DrawRectangleRec({x,y,(w-4)*fraction,4},energyColor(EnergyMedium(n)));y+=13;}
  if(m->item){text("REAL FEED / "+roundTo(analyse(m->item->food).mass,1)+" g",x,y,10,ink,true);y+=19;}if(m->sideOutput){text("SIDE / "+m->sideOutput->residue+" / "+roundTo(analyse(m->sideOutput->food).mass,1)+" g",x,y,10,orange,true);y+=19;}
  y+=paragraph(machineSpec(m->kind).description,x,y,w,10.5f,muted)+12;
  if(m->power.history.size()>1){text("MEASURED HISTORY",x,y,9.5f,muted,true);y+=18;float top=y,high=float(std::max(1.,*std::max_element(m->power.history.begin(),m->power.history.end())));DrawRectangleRec({x,top,w-4,56},Color{237,240,244,255});for(size_t n=1;n<m->power.history.size();n++){float a=float(n-1)/(m->power.history.size()-1),b=float(n)/(m->power.history.size()-1);DrawLineEx({x+a*(w-4),top+53-float(m->power.history[n-1])/high*50},{x+b*(w-4),top+53-float(m->power.history[n])/high*50},1.6f,green);}y+=67;}
 }
 y+=10;text("CONNECTED FACTORY GRID",x,y,10,ink,true);y+=22;text("GEN "+roundTo(factory.grid.generated,1)+" / REQUEST "+roundTo(factory.grid.requested,1),x,y,10,muted);y+=20;text("DELIVERED "+roundTo(factory.grid.delivered,1)+" / STORED "+roundTo(factory.grid.stored,1),x,y,10,muted);y+=20;text("HEAT/COLD RECOVERY "+roundTo(factory.grid.recoveredHeat,1),x,y,9.5f,muted);y+=20;text("SUN "+roundTo(solarDaylight(factory.tick)*100,0)+"% / WIND "+roundTo(windStrength(factory.tick)*100,0)+"%",x,y,9.5f,muted);y+=30;
 inspectorHeight=y-start;endClip();
}
}
