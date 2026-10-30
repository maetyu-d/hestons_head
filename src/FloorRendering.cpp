#include "Theme.h"
#include "ReagentWorks.h"
#include "Interface.h"
#include "MachineArt.h"
#include "GlassArt.h"
#include "Power.h"
#include "WaterWorks.h"
#include "PowerArt.h"
#include "RouteArt.h"
#include <algorithm>
#include <cmath>

namespace aftertaste {
namespace {
const Color paper={246,249,255,255},ink={22,41,75,255},muted={77,99,131,255},line={183,205,236,255},orange={240,43,43,255},white={255,255,255,255};
Color pigment(const std::string& hex){return theme::pigment(hex);}
Color utilityColor(UtilityKind k){static const std::array<Color,5> colors={Color{26,99,235,255},Color{240,43,43,255},Color{0,174,238,255},Color{13,183,76,255},Color{146,53,226,255}};return colors.at(size_t(k));}
Color tint(Color a,Color b,float f){f=std::clamp(f,0.f,1.f);return {static_cast<unsigned char>(a.r*(1-f)+b.r*f),static_cast<unsigned char>(a.g*(1-f)+b.g*f),static_cast<unsigned char>(a.b*(1-f)+b.b*f),255};}
Color foodColor(const MaterialView& v){double r=0,g=0,b=0,total=0;for(auto& [key,mass]:v.ingredients){auto c=pigment(ingredient(key).color);r+=c.r*mass;g+=c.g*mass;b+=c.b*mass;total+=mass;}if(total<=0)return muted;Color base={static_cast<unsigned char>(r/total),static_cast<unsigned char>(g/total),static_cast<unsigned char>(b/total),255};return tint(base,Color{91,51,31,255},float(std::clamp(v.metrics.brown*.6+v.metrics.scorch,0.,.9)));}
Vector2 centre(Rectangle r){return {r.x+r.width*.5f,r.y+r.height*.5f};}
void arrow(Vector2 at,int dir,float size,Color c){auto d=directions[dir%4];Vector2 a={at.x-d[0]*size*.5f,at.y-d[1]*size*.5f},b={at.x+d[0]*size*.5f,at.y+d[1]*size*.5f};DrawLineEx(a,b,1.5f,c);DrawLineEx(b,{b.x-d[0]*size*.4f-d[1]*size*.3f,b.y-d[1]*size*.4f+d[0]*size*.3f},1.5f,c);DrawLineEx(b,{b.x-d[0]*size*.4f+d[1]*size*.3f,b.y-d[1]*size*.4f-d[0]*size*.3f},1.5f,c);}
bool blocked(const Machine& m){return m.status=="Backed up"||m.status=="No outlet"||m.status=="Side outlet blocked"||m.status.find("utility exhausted")!=std::string::npos||m.status.rfind("Connect a ",0)==0||m.status=="Power limited"||m.status=="Water empty"||m.status=="Water supply empty"||m.status=="Electricity supply empty"||m.status=="Farm produce supply empty"||m.status=="Chemical supply empty"||m.status=="Needs cleaning"||m.status=="Water input empty"||m.status=="Chemical input empty"||m.status=="Electricity input empty"||m.status=="Check farm utilities";}
std::string stateLabel(const MaterialView& v){
 if(v.cropIntact)return v.cropDirt>.005?"soil "+roundTo(v.cropDirt*100,0)+"%":"clean / "+roundTo(v.metrics.temperature,0)+" C";
 if(v.vapor)return (v.fraction.empty()?"vapour":v.fraction+" vapour");
 if(!v.fraction.empty())return v.fraction+" / liquid";
 if(v.shell>.02)return "sh "+roundTo(v.shell*100,0)+"%";
 if(v.metrics.ice>.02)return "ice "+roundTo(v.metrics.ice*100,0)+"%";
 if(v.metrics.gel>.08)return "gel "+roundTo(v.metrics.gel*100,0)+"%";
 if(v.air>.04)return "air "+roundTo(v.air*100,0)+"%";
 if(v.metrics.phase=="emulsion")return "drop "+roundTo(v.droplet,0);
 if(v.metrics.brown>.02)return "br "+roundTo(v.metrics.brown*100,0)+"%";
 return roundTo(v.metrics.temperature,0)+" C";
}
// A small material chamber, entirely driven by the actual batch state.
void material(Rectangle r,const MaterialView& v,double time,bool active){
 Color base=foodColor(v);DrawRectangleRounded(r,.3f,5,tint(base,white,.07f));
 if(v.cropIntact&&!v.ingredients.empty()){
  auto crop=std::max_element(v.ingredients.begin(),v.ingredients.end(),[](const auto& a,const auto& b){return a.second<b.second;})->first;
  DrawRectangleRounded(r,.3f,5,tint(base,white,.78f));int count=r.width>r.height*1.8f?3:r.width>r.height*1.35f?2:1;float unit=std::min(r.height*.95f,r.width/count);
  for(int n=0;n<count;n++){float x=r.x+(r.width-unit*count)*.5f+n*unit;drawCropProduce({x,r.y+(r.height-unit)*.5f,unit,unit},crop,v.cropDirt,v.cropFreshness);}
  DrawRectangleRoundedLinesEx(r,.3f,5,1,v.metrics.temperature<=8?Color{26,99,235,255}:tint(base,ink,.35f));return;
 }
 float radius=std::min(r.width,r.height)*.18f;
 if(v.components.size()>1){double total=std::max(.000001,v.metrics.mass);float x=r.x;for(size_t i=0;i<v.components.size();i++){float w=float(v.components[i].first/total)*r.width;DrawRectangleRec({x,r.y+1,std::max(1.f,w-1),r.height-2},tint(base,i%2?white:ink,i%2?.35f:.13f));x+=w;}}
 // Coarse fat droplets become visibly smaller with homogenisation. Size is a
 // relative simulation value, not a real-world micrometre measurement.
 if(v.compounds[Fat]>.01){float drop=std::clamp(float(v.droplet/90),.05f,1.f);int count=std::clamp(int(3+(1-drop)*11),3,14);for(int n=0;n<count;n++){float x=r.x+r.width*(.12f+.76f*float((n*37)%101)/100),y=r.y+r.height*(.16f+.68f*float((n*61+19)%101)/100);DrawCircleV({x,y},std::max(.65f,radius*(.2f+drop*.9f)),tint(base,Color{255,235,153,255},.78f));}}
 if(v.metrics.gel>.035||v.shell>.03){Color network=tint(base,ink,.6f);float step=std::max(3.f,r.height/(3+float(v.metrics.gel)*3));for(float x=r.x+2;x<r.x+r.width-1;x+=step)DrawLineEx({x,r.y+2},{std::min(x+step,r.x+r.width-2),r.y+r.height-2},.8f,Fade(network,.7f));for(float y=r.y+3;y<r.y+r.height-2;y+=step)DrawLineEx({r.x+2,y},{r.x+r.width-2,y},.8f,Fade(network,.65f));}
 if(v.shell>.08||v.metrics.phase=="pearls"){for(int n=0;n<3;n++){Vector2 at={r.x+r.width*(.23f+.27f*n),r.y+r.height*.5f};DrawCircleV(at,radius*1.3f,tint(base,white,.5f));DrawCircleV(at,radius*.8f,base);DrawCircleLines(int(at.x),int(at.y),radius*1.3f,tint(base,ink,.5f));}}
 if(v.air>.015){int count=std::clamp(int(v.air*20),1,13);for(int n=0;n<count;n++){float wobble=active?float(std::sin(time*2+n))*.03f:0;Vector2 at={r.x+r.width*(.12f+.76f*float((n*41+3)%101)/100),r.y+r.height*(.18f+.64f*float((n*73+10)%101)/100+wobble)};DrawCircleV(at,std::max(1.f,radius*(.45f+.5f*float(n%3)/2)),Fade(white,.85f));}}
 if(v.vapor){for(int n=0;n<5;n++){float rise=active?float(std::fmod(time*.18+n*.19,1.)):float(n)*.18f;Vector2 p={r.x+r.width*(.18f+.16f*(n%5)),r.y+r.height*(.82f-rise*.70f)};DrawCircleV(p,std::max(1.2f,radius*(.18f+.08f*(n%3))),Fade(white,.9f));}}
 if(v.metrics.ice>.015){int count=std::clamp(int(v.metrics.ice*12),1,9);for(int n=0;n<count;n++){Vector2 p={r.x+r.width*(.13f+.74f*float((n*31+11)%101)/100),r.y+r.height*(.19f+.62f*float((n*57+5)%101)/100)};float z=std::max(1.7f,radius);DrawLineEx({p.x-z,p.y},{p.x+z,p.y},1,white);DrawLineEx({p.x,p.y-z},{p.x,p.y+z},1,white);DrawLineEx({p.x-z*.7f,p.y-z*.7f},{p.x+z*.7f,p.y+z*.7f},1,white);}}
 Color edge=v.metrics.temperature<8?Color{26,99,235,255}:v.metrics.temperature>35?orange:tint(base,ink,.35f);DrawRectangleRoundedLinesEx(r,.3f,5,v.metrics.temperature<8||v.metrics.temperature>35?1.8f:1,edge);
}
void glassFood(const std::vector<Vector2>& path,const MaterialView& v,float at,float diameter){
 Color c=foodColor(v);if(v.vapor)c=tint(c,white,.48f);float half=.13f;Vector2 last=glassPoint(path,std::max(0.f,at-half));
 for(int n=1;n<=18;n++){float t=std::clamp(at-half+2*half*n/18,0.f,1.f);auto q=glassPoint(path,t);if(!v.vapor||n%3!=0)DrawLineEx(last,q,std::max(1.5f,diameter*(v.vapor?.38f:.63f)),c);last=q;}
 auto centre=glassPoint(path,at);
 if(v.vapor){for(float offset:{-.08f,0.f,.08f}){auto q=glassPoint(path,std::clamp(at+offset,0.f,1.f));DrawCircleLines(int(q.x),int(q.y),std::max(1.3f,diameter*.25f),Fade(white,.9f));}}
 else if(v.air>.015||v.metrics.ice>.02){for(float t:{-.055f,.04f}){auto q=glassPoint(path,std::clamp(at+t,0.f,1.f));DrawCircleV(q,std::max(1.f,diameter*.16f),WHITE);}}
 if(v.metrics.gel>.06||v.shell>.05)DrawCircleLines(int(centre.x),int(centre.y),std::max(1.5f,diameter*.38f),tint(c,ink,.45f));
}
std::vector<Vector2> pathPart(const std::vector<Vector2>& p,float from,float to){std::vector<Vector2> result;for(int n=0;n<=24;n++)result.push_back(glassPoint(p,from+(to-from)*n/24));return result;}

}
void Interface::drawFactoryMap(){
 DrawRectangleRec(factoryView,{236,243,254,255});DrawRectangleLinesEx(factoryView,1,line);beginClip(factoryView);
 float tile=38*factoryZoom;double now=GetTime();
 auto first=factoryPosition({factoryView.x,factoryView.y}),last=factoryPosition({factoryView.x+factoryView.width,factoryView.y+factoryView.height});int stride=std::max(1,int(std::ceil(12/tile)));
 auto factoryArea=factory.enclosedFactoryTiles();
 for(auto p:factoryArea){if(p.x<first.x||p.x>last.x||p.y<first.y||p.y>last.y)continue;DrawRectangleRec(factoryTile(p),{218,248,228,255});}
 for(int x=int(std::floor(double(first.x)/stride))*stride;x<=last.x+1;x+=stride){auto r=factoryTile({x,0});DrawLineEx({r.x,factoryView.y},{r.x,factoryView.y+factoryView.height},1,{208,224,247,255});}
 for(int y=int(std::floor(double(first.y)/stride))*stride;y<=last.y+1;y+=stride){auto r=factoryTile({0,y});DrawLineEx({factoryView.x,r.y},{factoryView.x+factoryView.width,r.y},1,{208,224,247,255});}
 drawEnergyRoutes();
 auto inputs=[&](Position p,const Machine& m){std::vector<int> result;for(int d=0;d<4;d++){auto q=nextPosition(p,d);auto src=factory.at(q);if(src&&src->kind!=MachineKind::Wall&&!isUtilityEquipment(src->kind)&&!isSensor(src->kind)&&!isFarmInfrastructure(src->kind)&&(nextPosition(q,src->dir)==p||(hasSideOutlet(src->kind)&&nextPosition(q,(src->dir+1)%4)==p)))result.push_back(d);}if(result.empty()&&!isUtilityEquipment(m.kind)&&!isSensor(m.kind)&&!isFarmInfrastructure(m.kind)&&m.kind!=MachineKind::Wall)result.push_back((m.dir+2)%4);return result;};
 auto portDirection=[](Position from,Position to){if(to.x>from.x)return 0;if(to.y>from.y)return 1;if(to.x<from.x)return 2;return 3;};
 auto farmColor=[](FarmFlowKind kind){switch(kind){case FarmFlowKind::Seed:return Color{165,123,55,255};case FarmFlowKind::Nutrients:return Color{26,99,235,255};case FarmFlowKind::Light:return Color{255,211,22,255};case FarmFlowKind::Pollination:return Color{255,211,22,255};case FarmFlowKind::Harvest:return Color{13,183,76,255};case FarmFlowKind::Storage:return Color{13,183,76,255};case FarmFlowKind::Compost:return Color{129,103,64,255};}return ink;};
 // Actual farm connections share the floor with food routes. Their thinner,
 // dashed paths identify seed and growing inputs separately from conveyors.
 if(floorFlow&&tile>=13)for(const auto& [p,m]:factory.machines){
  if(!isFarmInfrastructure(m.kind)&&m.kind!=MachineKind::CompostBin)continue;
  for(int dir=0;dir<4;dir++){Position q=nextPosition(p,dir);auto bed=factory.at(q);if(!bed||!isFarmBed(bed->kind))continue;FarmFlowKind kind;
   if(m.kind==MachineKind::SeedHopper&&dir==m.dir)kind=FarmFlowKind::Seed;
   else if(m.kind==MachineKind::Fertigation)kind=FarmFlowKind::Nutrients;
   else if(m.kind==MachineKind::GrowLight)kind=FarmFlowKind::Light;
   else if(m.kind==MachineKind::PollinatorHive&&(bed->ingredient=="tomato"||bed->ingredient=="pepper"||bed->ingredient=="berry"||bed->ingredient=="citrus"))kind=FarmFlowKind::Pollination;
   else if(m.kind==MachineKind::CompostBin)kind=FarmFlowKind::Compost;else continue;
   auto a=centre(factoryTile(p)),b=centre(factoryTile(q));Color color=Fade(farmColor(kind),m.enabled?.65f:.22f);
   for(int n=0;n<7;n++){float t=n/7.f,u=(n+.55f)/7.f;DrawLineEx({a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t},{a.x+(b.x-a.x)*u,a.y+(b.y-a.y)*u},std::max(1.f,tile*.035f),color);}
   arrow({(a.x+b.x)*.5f,(a.y+b.y)*.5f},dir,std::max(4.f,tile*.14f),color);
  }
 }
 if(floorFlow&&tile>=13)for(const auto& [p,m]:factory.machines)if(isFarmBed(m.kind))for(int dir=0;dir<4;dir++){Position q=nextPosition(p,dir);auto harvester=factory.at(q);if(!harvester||harvester->kind!=MachineKind::Harvester||nextPosition(q,(harvester->dir+2)%4)!=p)continue;auto a=centre(factoryTile(p)),b=centre(factoryTile(q));DrawLineEx(a,b,std::max(2.f,tile*.1f),Fade(farmColor(FarmFlowKind::Harvest),.42f));arrow({(a.x+b.x)*.5f,(a.y+b.y)*.5f},dir,std::max(5.f,tile*.17f),farmColor(FarmFlowKind::Harvest));}
 // Chemical glass is a material route. AC contact rails and water utilities
 // must never acquire a second, fictitious food connection through the same gap.
 auto materialInput=[](MachineKind k){
  if(isEnergyMachine(k))return acceptsEnergyFuel(k);
  return k!=MachineKind::Wall&&k!=MachineKind::Feeder&&k!=MachineKind::RecipeKitchen&&k!=MachineKind::Harvester&&!isUtilityEquipment(k)&&!isSensor(k)&&!isFarmInfrastructure(k);
 };
 auto materialOutput=[](MachineKind k,bool side){
  if(side)return hasSideOutlet(k);
  if(isReagentMachine(k))return k!=MachineKind::ReagentIntake&&k!=MachineKind::ReagentRecovery&&k!=MachineKind::ReagentAnalyzer;
  if(isEnergyMachine(k))return k==MachineKind::Macerator;
  return k!=MachineKind::Wall&&k!=MachineKind::Bin&&k!=MachineKind::CompostBin&&!isUtilityEquipment(k)&&!isSensor(k)&&!isFarmInfrastructure(k);
 };
 std::set<Position> chemicalGlass;std::vector<Position> chemicalQueue;
 for(const auto& [p,m]:factory.machines)if((isReagentMachine(m.kind)&&m.kind!=MachineKind::ReagentAnalyzer)||(m.item&&(!m.item->reagent.key.empty()||m.item->reagent.mineral))){chemicalGlass.insert(p);chemicalQueue.push_back(p);}
 for(size_t n=0;n<chemicalQueue.size();n++)for(int d=0;d<4;d++){auto q=nextPosition(chemicalQueue[n],d);auto m=factory.at(q);if(m&&isGlassConduit(m->kind)&&chemicalGlass.insert(q).second)chemicalQueue.push_back(q);}
 auto chemicalRoute=[&](Position a,Position b){return chemicalGlass.count(a)||chemicalGlass.count(b);};
 if(tile>=13)for(auto& [p,m]:factory.machines)for(int side=0;side<2;side++){
  if(!materialOutput(m.kind,side))continue;int dir=(m.dir+side)%4;auto dest=nextPosition(p,dir);auto target=factory.at(dest);if(!target||!materialInput(target->kind))continue;
  Vector2 a=centre(factoryTile(p)),b=centre(factoryTile(dest)),edge{(a.x+b.x)*.5f,(a.y+b.y)*.5f};bool chemical=chemicalRoute(p,dest);
  if(chemical||isLabGlassware(m.kind)||isLabGlassware(target->kind)){
   bool sourcePipe=isGlassConduit(m.kind),targetPipe=isGlassConduit(target->kind);
   if(!(sourcePipe&&targetPipe))drawGlassWall({sourcePipe?edge:a,targetPipe?edge:b},std::max(3.f,tile*(chemical?.125f:.18f)),false,false,chemical);
  }else if(floorFlow){Color c{147,174,173,255};const auto& batch=side?m.sideOutput:m.item;if(batch){auto i=floorTelemetry.samples.find(batch->id);if(i!=floorTelemetry.samples.end())c=tint(foodColor(i->second.material),ink,.16f);}DrawLineEx(a,b,std::max(2.f,tile*.13f),Fade(c,.6f));arrow(edge,dir,std::max(5.f,tile*.17f),blocked(m)?orange:ink);}
 }
 // Assays are a dotted instrument lead, distinct from either fluid or power.
 if(tile>=20)for(const auto& [p,m]:factory.machines)if(m.kind==MachineKind::ReagentAnalyzer){auto q=nextPosition(p,m.dir);auto target=factory.at(q);if(!target||!acceptsReagentMaterial(target->kind))continue;auto a=centre(factoryTile(p)),b=centre(factoryTile(q));for(int n=0;n<8;n++){float t=(n+.3f)/8;DrawCircleV({a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t},std::max(.8f,tile*.012f),Color{146,103,155,255});}}

 auto node=[&](Position pos,const Machine& m,bool ghost){
  Rectangle full=factoryTile(pos);if(!CheckCollisionRecs(full,factoryView))return;Color tone=theme::machinePaint(m.kind);float pad=std::max(1.f,tile*.065f);Rectangle r={full.x+pad,full.y+pad,full.width-2*pad,full.height-2*pad};
  if(tile<13){DrawRectangleRec(r,ghost?Fade(tone,.4f):tone);return;}
  if(m.kind==MachineKind::Wall){float inset=std::max(2.f,tile*.055f);Color dark={72,95,132,255},face={225,234,249,255},cap={162,190,229,255};if(ghost){dark=Fade(dark,.45f);face=Fade(face,.45f);cap=Fade(cap,.45f);}Rectangle slab={full.x+inset,full.y+inset,full.width-inset*2,full.height-inset*2};DrawRectangleRec(slab,dark);Rectangle top={slab.x+1,slab.y+1,slab.width-2,std::max(2.f,slab.height*.20f)};DrawRectangleRec(top,cap);Rectangle brick={slab.x+2,top.y+top.height+1,slab.width-4,slab.height-top.height-5};DrawRectangleRec(brick,face);if(tile>=24){DrawLineEx({brick.x,brick.y+brick.height*.52f},{brick.x+brick.width,brick.y+brick.height*.52f},1,dark);DrawLineEx({brick.x+brick.width*.5f,brick.y},{brick.x+brick.width*.5f,brick.y+brick.height*.52f},1,dark);DrawLineEx({brick.x+brick.width*.27f,brick.y+brick.height*.52f},{brick.x+brick.width*.27f,brick.y+brick.height},1,dark);DrawLineEx({brick.x+brick.width*.76f,brick.y+brick.height*.52f},{brick.x+brick.width*.76f,brick.y+brick.height},1,dark);}return;}
  if(m.kind==MachineKind::UtilityLine){Color service=utilityColor(m.utility);auto c=centre(full);float width=std::max(2.2f,tile*.12f);bool connected=false;for(int d=0;d<4;d++){auto neighbor=factory.at(nextPosition(pos,d));if(!neighbor||!isUtilityEquipment(neighbor->kind)||neighbor->utility!=m.utility)continue;connected=true;auto delta=directions[d];Vector2 end{c.x+delta[0]*tile*.48f,c.y+delta[1]*tile*.48f};if(m.utility==UtilityKind::Water)drawInfrastructureRoute({c,end},EnergyMedium::CleanWater,tile,!ghost);else DrawLineEx(c,end,width,ghost?Fade(service,.55f):service);}DrawCircleV(c,std::max(3.f,tile*.10f),ghost?Fade(service,.55f):service);DrawCircleLines(int(c.x),int(c.y),std::max(3.f,tile*.10f),ink);if(!connected)DrawCircleV(c,std::max(1.f,tile*.035f),white);return;}
  if(isGlassConduit(m.kind)){
   auto in=ghost?std::vector<int>{(m.dir+2)%4}:inputs(pos,m);float diameter=std::max(3.f,tile*(m.kind==MachineKind::GlassCoil?.115f:.18f));
   for(int inlet:in)drawGlassWall(glassPath(full,m.kind,inlet,m.dir),diameter,ghost,false,chemicalGlass.count(pos)>0);
   MachineAppearance look{m.kind,m.dir,false,ghost,false,now};look.closed=!m.enabled;
   if(!ghost&&m.item&&!(floorFlow&&floorTelemetry.moving(m.item->id,now))){auto i=floorTelemetry.samples.find(m.item->id);if(i!=floorTelemetry.samples.end()){float progress=m.kind==MachineKind::GlassCoil?.06f+.88f*float(m.progress)/std::max(1,m.duration):.5f;glassFood(glassPath(full,m.kind,in.front(),m.dir),i->second.material,progress,diameter);}}
   drawMachineMechanism(full,look);
   if(!ghost){if(m.kind==MachineKind::GlassValve&&tile>=38){std::string label=m.enabled?"OPEN":"CLOSED";float fs=8;text(label,full.x+(tile-width(label,fs,true))*.5f,full.y+tile-10,fs,m.enabled?muted:orange,true);}
    if(m.kind==MachineKind::GlassCoil&&m.progress>0)DrawRectangleRec({full.x+3,full.y+tile-3,(tile-6)*float(m.progress)/std::max(1,m.duration),2},tone);
    if(blocked(m)||(!m.enabled&&m.kind!=MachineKind::GlassValve)){auto d=directions[m.dir];Vector2 c=centre(full),g={c.x+d[0]*tile*.47f,c.y+d[1]*tile*.47f};DrawLineEx({g.x-d[1]*tile*.16f,g.y+d[0]*tile*.16f},{g.x+d[1]*tile*.16f,g.y-d[0]*tile*.16f},3,orange);}
   }return;
  }
  if(isEnergyMachine(m.kind)){MachineAppearance look;look.kind=m.kind;look.direction=m.dir;look.labFlux=m.power.lab.flux;look.labVacuum=m.power.lab.vacuum;look.labStability=m.power.lab.stability;look.labCooling=m.power.lab.cooling;look.labArmed=m.power.lab.armed;look.active=factory.running&&m.enabled&&!m.power.tripped&&(m.power.generated>.001||m.power.flow>.001||m.power.rpm>20||m.power.lab.flux>.01);look.time=factory.tick+(factory.running?factoryAccumulator/.55:0);look.ghost=ghost;look.closed=!m.enabled||m.power.tripped;double stored=0;for(auto value:m.power.store)stored+=value;look.progress=stored/std::max(1.,m.power.capacity);if(isReagentMachine(m.kind)){look.active=factory.running&&m.enabled&&m.power.delivered>.001;look.reagentFill=m.item?std::min(1.,analyse(m.item->food).mass/100):m.reagentProcess.minerals/500;look.reagentPurity=m.item?reagentPurity(*m.item):1;look.reagentKey=m.item?m.item->reagent.key:m.ingredient;}if(isWaterMachine(m.kind)){auto q=waterSample(m);if(q.volume>1e-8){look.waterDirt=q.solids/q.volume;look.waterSalt=q.salt/q.volume;look.waterBio=q.bio/q.volume;}}drawMachineHousing(r,look);if(m.item){auto sample=floorTelemetry.samples.find(m.item->id);if(sample!=floorTelemetry.samples.end())material(machineFoodWindow(r,m.kind,m.dir),sample->second.material,now,look.active);}drawMachineMechanism(r,look);if(!ghost&&tile>=32){std::string label=m.power.generated>.01?roundTo(m.power.generated,1)+(m.kind==MachineKind::SolarPanel||m.kind==MachineKind::Calorimeter?" DC":" AC"):m.power.tripped?"TRIP":stored>.01?roundTo(stored,0)+" u":m.power.flow>.01?roundTo(m.power.flow,1):machineSpec(m.kind).code;if(isWaterMachine(m.kind)){label=m.power.flow>.01?roundTo(m.power.flow,1)+" L":stored>.01?roundTo(stored,0)+" L":machineSpec(m.kind).code;if(m.power.flow>.01&&m.waterProcess.sample.volume>1e-8){const auto& q=m.waterProcess.sample;if(m.kind==MachineKind::MembraneFilter||m.kind==MachineKind::ReedReclaimer)label=roundTo(m.power.flow,1)+">"+roundTo(q.volume,1)+"L";if(m.kind==MachineKind::SandFilter)label="SILT "+roundTo(q.solids/q.volume,2);if(m.kind==MachineKind::UVSterilizer)label="BIO "+roundTo(q.bio/q.volume,3);}}if(isReagentMachine(m.kind)){label=m.item&&!m.item->reagent.key.empty()?ingredient(m.item->reagent.key).code+" "+roundTo(reagentPurity(*m.item)*100,0)+"%":m.kind==MachineKind::MineralHopper?roundTo(m.reagentProcess.minerals,0)+"g":machineSpec(m.kind).code;}float size=std::clamp(tile*.13f,7.f,10.f),ww=std::min(r.width,width(label,size,true)+4);DrawRectangleRec({r.x,full.y+tile-size-2,ww,size+2},Fade(white,.93f));text(fit(label,ww-2,size,true),r.x+1,full.y+tile-size-2,size,ink,true);}if(m.power.tripped)DrawCircleV({r.x+3,r.y+3},std::max(2.f,tile*.07f),orange);return;}
  bool belt=m.kind==MachineKind::Belt;
  bool processing=factory.running&&m.enabled&&m.item&&(m.status=="Processing"||m.power.delivered>.001);
  MachineAppearance look{m.kind,m.dir,processing,ghost,false,now*factorySpeed};look.utility=m.utility;look.progress=isFarmBed(m.kind)?m.farmGrowth:double(m.progress)/std::max(1,m.duration);look.cropPlanted=isFarmBed(m.kind)&&m.farmPlanted;
  look.forwarding=m.forward;look.crop=m.ingredient;look.cropHealth=m.farmHealth;look.cropMoisture=m.farmMoisture;
  if(factory.running&&m.enabled)for(const auto& flow:floorTelemetry.farmFlows)if(flow.from==pos&&now<flow.start+flow.duration)look.active=true;
  drawMachineHousing(r,look);
  if(ghost){drawMachineMechanism(r,look);return;}
  auto drawSlot=[&](const std::optional<Batch>& batch,int slot){
   if(!batch)return;auto i=floorTelemetry.samples.find(batch->id);if(i==floorTelemetry.samples.end())return;
   if(!(floorFlow&&floorTelemetry.moving(batch->id,now)))material(machineFoodWindow(r,m.kind,m.dir,slot,bool(m.extra)),i->second.material,now,processing);
  };
  drawSlot(m.item,0);drawSlot(m.extra,1);drawSlot(m.sideOutput,2);
  drawMachineMechanism(r,look);
  if(tile>=27&&!isUtilityEquipment(m.kind)&&!isSensor(m.kind)&&!isFarmInfrastructure(m.kind)){auto c=centre(full);auto port=[&](int d,Color color){auto delta=directions[(d%4+4)%4];Vector2 point={c.x+delta[0]*tile*.465f,c.y+delta[1]*tile*.465f};DrawCircleV(point,std::max(2.f,tile*.045f),color);DrawCircleLines(int(point.x),int(point.y),std::max(2.f,tile*.045f),white);};for(int d:inputs(pos,m))port(d,Color{40,176,193,255});port(m.dir,Color{25,75,87,255});if(hasSideOutlet(m.kind))port((m.dir+1)%4,Color{239,124,67,255});}
  // Quantities and food state remain available, while equipment type is shown
  // by its silhouette and mechanism instead of an abbreviation on a card.
  if(!belt&&m.kind!=MachineKind::Wall&&tile>=38){std::string label;
   if(m.kind==MachineKind::Feeder){double grams=m.dose;if(m.item){auto i=floorTelemetry.samples.find(m.item->id);if(i!=floorTelemetry.samples.end())grams=i->second.material.metrics.mass;}label=ingredient(m.ingredient).code+roundTo(grams,0)+"g";}
   else if(isFarmBed(m.kind))label=m.farmPlanted?ingredient(m.ingredient).code+" "+(m.farmGrowth>=1?"RIPE":std::to_string(int(m.farmGrowth*100))+"%"):"EMPTY BED";
   else if(m.kind==MachineKind::SeedHopper)label=ingredient(m.ingredient).code+" SEED";
   else if(m.kind==MachineKind::Harvester)label=m.item?roundTo(analyse(m.item->food).mass,0)+"g":"HARVEST";
   else if(m.kind==MachineKind::ProduceIntake){auto i=factory.farmInventory.find(m.ingredient);label=m.forward?"TO FACTORY":m.lastOutput?ingredient(m.ingredient).code+" "+roundTo(i==factory.farmInventory.end()?0:i->second,2)+"kg":"INTAKE";}
   else if(m.item&&m.item->cropBatch&&m.kind==MachineKind::Washer)label="SOIL "+roundTo(m.item->cropDirt*100,0)+"%";
   else if(m.item&&m.item->cropBatch&&m.kind==MachineKind::Grader)label="GRADE "+std::string(m.item->cropQuality>.8?"A":m.item->cropQuality>.55?"B":"C");
   else if(m.item&&m.item->cropBatch&&m.kind==MachineKind::ColdStore)label=roundTo(analyse(m.item->food).temperature,0)+" C / "+roundTo(m.item->cropFreshness*100,0)+"%";
   else if(isDosingMachine(m.kind))label="+"+ingredient(m.ingredient).code+roundTo(m.dose,1);
   else if(m.item){auto i=floorTelemetry.samples.find(m.item->id);if(i!=floorTelemetry.samples.end())label=stateLabel(i->second.material);}
   else if(isTastingDock(m.kind))label=m.forward?"FORWARD":m.automatic?"SERVE":"HOLD";
   else if(m.kind==MachineKind::UtilityStation)label=utilityCode(m.utility)+" "+roundTo(m.utilityCapacity,0);
   else if(m.kind==MachineKind::SoilProbe)label="SOIL "+roundTo(m.sensorValue*100,0)+"%";
   else if(m.kind==MachineKind::PHProbe||m.kind==MachineKind::TemperatureLogger||m.kind==MachineKind::DropletViewer||m.kind==MachineKind::AromaSensor)label=m.sensorBatch?roundTo(m.sensorValue,m.kind==MachineKind::PHProbe?2:1):"SCAN";
   if(!label.empty()){float size=std::clamp(tile*.15f,8.f,11.f),w=std::min(r.width,width(label,size,true)+7);Rectangle tag={r.x+(r.width-w)*.5f,full.y+full.height-size-2,w,size+2};DrawRectangleRounded(tag,.3f,4,Fade(white,.94f));text(fit(label,w-4,size,true),tag.x+2,tag.y,size,ink,true);}
  }
  if((m.progress>0||(isFarmBed(m.kind)&&m.farmPlanted))&&!belt){float fraction=float(isFarmBed(m.kind)?m.farmGrowth:clamp(double(m.progress)/std::max(1,m.duration),0,1));DrawRectangleRec({r.x,full.y+full.height-2,r.width*fraction,2},tone);}
  if(isFarmBed(m.kind)&&m.farmPlanted){Color health=m.farmHealth<.55?orange:Color{66,151,87,255};DrawCircleV({r.x+r.width-3,r.y+4},std::max(2.f,tile*.05f),health);if(tile>=28&&tile<38){std::string growth=roundTo(m.farmGrowth*100,0)+"%";text(growth,r.x,r.y+r.height-7,7,ink,true);}}
  if(!m.enabled){DrawRectangleRec(r,Fade(paper,.58f));DrawLineEx({r.x+r.width*.28f,r.y+r.height*.28f},{r.x+r.width*.72f,r.y+r.height*.72f},2,ink);}
  if(blocked(m)){auto d=directions[m.status=="Side outlet blocked"?(m.dir+1)%4:m.dir];Vector2 c=centre(full),g={c.x+d[0]*tile*.47f,c.y+d[1]*tile*.47f};DrawLineEx({g.x-d[1]*tile*.18f,g.y+d[0]*tile*.18f},{g.x+d[1]*tile*.18f,g.y-d[0]*tile*.18f},3,orange);}
  if(m.status=="Needs cleaning"||m.status=="Power limited")DrawCircleV({r.x+3,r.y+3},std::max(2.f,tile*.06f),orange);
 };
 for(auto& [p,m]:factory.machines)node(p,m,false);
 if(floorFlow&&tile>=13)for(const auto& flow:floorTelemetry.farmFlows){float t=float(std::clamp((now-flow.start)/flow.duration,0.,1.));if(t>=1)continue;auto a=centre(factoryTile(flow.from)),b=centre(factoryTile(flow.to));Color color=farmColor(flow.kind);
  if(flow.kind==FarmFlowKind::Storage){DrawCircleLines(int(a.x),int(a.y),tile*(.18f+.36f*t),Fade(color,1-t));if(tile>=32){std::string caption="+"+roundTo(flow.amount,0)+"g STORED";float fs=std::clamp(tile*.16f,8.f,11.f);text(caption,a.x-width(caption,fs,true)*.5f,a.y-tile*.50f-t*8,fs,color,true);}continue;}
  Vector2 point={a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t};
  if(flow.kind==FarmFlowKind::Harvest){DrawLineEx(a,b,std::max(2.f,tile*.08f),Fade(color,1-t));continue;}
  if(flow.kind==FarmFlowKind::Light){DrawLineEx(a,b,std::max(1.f,tile*.03f),Fade(color,.22f));DrawCircleV(point,std::max(1.f,tile*.035f),color);}
  else if(flow.kind==FarmFlowKind::Pollination){point.y+=std::sin(t*6.28f)*tile*.12f;DrawCircleV(point,std::max(2.f,tile*.045f),color);DrawCircleV({point.x+2,point.y-2},std::max(1.f,tile*.025f),white);}
  else{DrawCircleV(point,std::max(2.f,tile*.055f),color);if(flow.kind==FarmFlowKind::Nutrients)DrawCircleV({point.x,point.y-2},std::max(1.f,tile*.025f),white);}
 }
 if(floorFlow)for(auto& motion:floorTelemetry.motions){float t=float(std::clamp((now-motion.start)/motion.duration,0.,1.));if(t>=1)continue;auto src=factory.at(motion.from),dst=factory.at(motion.to);Vector2 a=centre(factoryTile(motion.from)),b=centre(factoryTile(motion.to));
  if(src&&dst&&(chemicalRoute(motion.from,motion.to)||isLabGlassware(src->kind)||isLabGlassware(dst->kind))){std::vector<Vector2> route;
   if(isGlassConduit(src->kind))route=pathPart(glassPath(factoryTile(motion.from),src->kind,inputs(motion.from,*src).front(),src->dir),src->kind==MachineKind::GlassCoil?.94f:.5f,1);
   else route={a,{(a.x+b.x)*.5f,(a.y+b.y)*.5f}};
   if(isGlassConduit(dst->kind)){auto p=pathPart(glassPath(factoryTile(motion.to),dst->kind,portDirection(motion.to,motion.from),dst->dir),0,dst->kind==MachineKind::GlassCoil?.06f:.5f);route.insert(route.end(),p.begin(),p.end());}else route.push_back(b);
   glassFood(route,motion.material,t,std::max(3.f,tile*.15f));
  }else{float u=t*t*(3-2*t);Vector2 c={a.x+(b.x-a.x)*u,a.y+(b.y-a.y)*u};DrawLineEx(a,c,std::max(1.5f,tile*.07f),Fade(foodColor(motion.material),.6f));float w=std::max(7.f,tile*(motion.material.cropIntact?.67f:.55f)),h=std::max(6.f,tile*(motion.material.cropIntact?.67f:.32f));material({c.x-w*.5f,c.y-h*.5f,w,h},motion.material,now,false);}
 }
 bool over=CheckCollisionPointRec(pointer(),factoryView)&&!CheckCollisionPointRec(pointer(),factoryMiniMap);
 if(over&&!factoryOverlay){if(factoryStamp){bool okay=factory.canStamp(*factoryStamp,factoryHovered);for(auto& p:factoryStamp->parts)node({factoryHovered.x+p.at.x,factoryHovered.y+p.at.y},p.machine,true);auto r=factoryTile(factoryHovered);DrawRectangleLinesEx({r.x,r.y,r.width*factoryStamp->width,r.height*factoryStamp->height},2,okay?Color{36,157,110,255}:orange);}
  else if(!factorySelecting){auto r=factoryTile(factoryHovered);DrawRectangleLinesEx(r,2,factoryErasing?orange:Color{62,139,151,255});if(factoryTab==0&&(!factory.at(factoryHovered)||isEnergyConduit(factoryTool))&&!factoryErasing&&!factoryCopying){Machine m;m.kind=factoryTool;m.dir=factoryDirection;node(factoryHovered,m,true);}}}
 if(factorySelecting){Position a={std::min(factoryDragStart.x,factoryHovered.x),std::min(factoryDragStart.y,factoryHovered.y)};auto r=factoryTile(a);r.width*=std::abs(factoryDragStart.x-factoryHovered.x)+1;r.height*=std::abs(factoryDragStart.y-factoryHovered.y)+1;DrawRectangleRec(r,Fade(Color{33,153,178,255},.12f));DrawRectangleLinesEx(r,2,Color{33,153,178,255});}
 auto selected=factoryTile(factorySelected);if(factory.at(factorySelected)&&CheckCollisionRecs(selected,factoryView))DrawRectangleLinesEx({selected.x+1,selected.y+1,selected.width-2,selected.height-2},2,orange);
 // An on-floor annotation keeps cause and effect next to the real equipment.
 Rectangle annotation{};bool inspectHovered=over&&factoryHovered!=factorySelected&&factory.at(factoryHovered);Position focus=inspectHovered?factoryHovered:factorySelected;auto machine=factory.at(focus);auto anchor=factoryTile(focus);
 // The inspector already describes the selected machine. Keep the floor's
 // hover card for other machines only, so it never duplicates or obscures focus.
 if(floorFlow&&inspectHovered&&machine&&tile>=21&&CheckCollisionRecs(anchor,factoryView)&&!factoryStamp&&!factorySelecting&&!factoryOverlay){
  auto change=floorTelemetry.changes.find(focus);const FloorSample* sample=nullptr;if(machine->item){auto i=floorTelemetry.samples.find(machine->item->id);if(i!=floorTelemetry.samples.end())sample=&i->second;}
  bool compact=factoryView.width<450;float w=std::min(compact?204.f:232.f,factoryView.width-24),h=sample||isFarmBed(machine->kind)||change!=floorTelemetry.changes.end()?68.f:50.f;
  std::vector<Rectangle> candidates={{anchor.x,anchor.y-h-12,w,h},{anchor.x,anchor.y+tile+12,w,h},{anchor.x-w-12,anchor.y,w,h},{anchor.x+tile+12,anchor.y,w,h},{factoryView.x+8,factoryView.y+2,w,h}};
  // Compact annotations fit between production rows. Score the visible machine
  // bodies so empty tile margins can be used without covering actual equipment.
  std::set<float> rows;for(auto& [p,m]:factory.machines){auto r=factoryTile(p);if(CheckCollisionRecs(r,factoryView))rows.insert(r.y+r.height-tile*.065f+2);}for(float y:rows)for(float x:{anchor.x,anchor.x-w-12,factoryView.x+8})candidates.push_back({x,y,w,h});
  Rectangle box{};float best=1e30f;for(auto c:candidates){c.x=std::clamp(c.x,factoryView.x+8,factoryView.x+factoryView.width-w-8);c.y=std::clamp(c.y,factoryView.y+2,factoryView.y+factoryView.height-h-54);float score=(CheckCollisionRecs(c,anchor)?10000.f:0.f)+.025f*(std::abs(centre(c).x-centre(anchor).x)+std::abs(centre(c).y-centre(anchor).y));for(auto& [p,m]:factory.machines){auto r=factoryTile(p);float pad=std::max(1.f,tile*.065f);r={r.x+pad,r.y+pad,r.width-2*pad,r.height-2*pad};if(CheckCollisionRecs(c,r)){auto overlap=GetCollisionRec(c,r);score+=overlap.width*overlap.height;}}if(score<best){best=score;box=c;}}
  annotation=box;auto a=centre(anchor);Vector2 b={std::clamp(a.x,box.x+4,box.x+w-4),std::clamp(a.y,box.y+4,box.y+h-4)};if(std::abs(a.x-b.x)+std::abs(a.y-b.y)<tile*3)DrawLineEx(a,b,1.2f,orange);DrawRectangleRec({box.x+2,box.y+3,w,h},Fade(ink,.09f));DrawRectangleRec(box,Fade(white,.98f));DrawRectangleLinesEx(box,1,orange);
  float x=box.x+9,y=box.y+6;text(fit(machineSpec(machine->kind).name+" / tile "+positionName(focus),w-18,10.5f,true),x,y,10.5f,ink,true);y+=15;
  std::string status=(!machine->enabled?"OFF / ":"")+machine->status;
  if(sample){const auto& v=sample->material;std::string state=v.vapor?upper(v.fraction)+" VAPOUR":!v.fraction.empty()?upper(v.fraction)+" LIQUID":upper(v.metrics.phase);text(fit(roundTo(v.metrics.mass,1)+" g  "+state+"  "+roundTo(v.metrics.temperature,0)+" C",w-18,10.5f,true),x,y,10.5f,ink,true);y+=15;
   if(change!=floorTelemetry.changes.end()){bool current=machine->item&&machine->item->id==change->second.batch;text(fit(std::string(current?"NOW / ":"LAST / ")+change->second.title,w-18,9.5f,true),x,y,9.5f,Color{30,122,104,255},true);}
   else{text(fit("pH "+roundTo(v.metrics.pH,2)+"  /  "+stateLabel(v),w-18,9.5f),x,y,9.5f,muted);}
   y+=14;text(fit(status,w-18,9),x,y,9,blocked(*machine)?orange:muted);
  }else if(isFarmBed(machine->kind)){
   text(fit(ingredient(machine->ingredient).name+" / "+roundTo(machine->farmGrowth*100,0)+"% grown",w-18,10,true),x,y,10,ink,true);y+=15;text("WATER "+roundTo(machine->farmMoisture*100,0)+"% / HEALTH "+roundTo(machine->farmHealth*100,0)+"%",x,y,9,muted);y+=14;text(fit(status,w-18,9),x,y,9,muted);
  }else{
   text(fit(status,w-18,9.5f,true),x,y,9.5f,blocked(*machine)?orange:muted,true);y+=14;
   if(change!=floorTelemetry.changes.end())text(fit("LAST / "+change->second.title,w-18,9.5f,true),x,y,9.5f,Color{30,122,104,255},true);
   else text(fit(machine->kind==MachineKind::Feeder?roundTo(machine->dose,1)+" g "+ingredient(machine->ingredient).name:"No active food in this chamber",w-18,9),x,y,9,muted);
  }
 }
 endClip();
 float miniW=factoryView.width<450?64:94,miniH=factoryView.width<450?44:64;factoryMiniMap={factoryView.x+factoryView.width-miniW-9,factoryView.y+9,miniW,miniH};DrawRectangleRec(factoryMiniMap,Fade(white,.94f));DrawRectangleLinesEx(factoryMiniMap,1,line);
 int minx=-4,maxx=4,miny=-4,maxy=4;if(!factory.machines.empty()){auto p=factory.machines.begin()->first;minx=p.x-2;maxx=p.x+2;miny=p.y-2;maxy=p.y+2;}for(auto& [p,m]:factory.machines){minx=std::min(minx,p.x-2);maxx=std::max(maxx,p.x+2);miny=std::min(miny,p.y-2);maxy=std::max(maxy,p.y+2);}for(auto& [p,m]:factory.machines){float xx=factoryMiniMap.x+5+float(p.x-minx)/(maxx-minx)*(miniW-10),yy=factoryMiniMap.y+5+float(p.y-miny)/(maxy-miny)*(miniH-10);DrawRectangle(int(xx),int(yy),2,2,theme::machinePaint(m.kind));}
 float px=factoryMiniMap.x+5+(factoryCamera.x-minx)/(maxx-minx)*(miniW-10),py=factoryMiniMap.y+5+(factoryCamera.y-miny)/(maxy-miny)*(miniH-10);DrawCircleLines(int(px),int(py),4,orange);hit(factoryMiniMap,[this,minx,maxx,miny,maxy]{auto m=pointer();factoryCamera={minx+(m.x-factoryMiniMap.x)/factoryMiniMap.width*(maxx-minx),miny+(m.y-factoryMiniMap.y)/factoryMiniMap.height*(maxy-miny)};},true,"Click to jump to another production district");
 Position readout=over?factoryHovered:Position{int(std::floor(factoryCamera.x)),int(std::floor(factoryCamera.y))};if(!CheckCollisionRecs(annotation,{factoryView.x+8,factoryView.y+8,180,18}))text(std::string(over?"TILE ":"VIEW ")+positionName(readout)+"   "+roundTo(factoryZoom*100,0)+"%",factoryView.x+12,factoryView.y+12,10,muted,true);
 if(floorFlow){bool hasWalls=std::any_of(factory.machines.begin(),factory.machines.end(),[](const auto& item){return item.second.kind==MachineKind::Wall;});bool hasFarm=!floorTelemetry.farmReadings.empty();bool hasPower=!factory.energyLinks.empty();bool hasWater=std::any_of(factory.machines.begin(),factory.machines.end(),[](const auto& p){return isWaterMachine(p.second.kind);});bool hasChem=std::any_of(factory.machines.begin(),factory.machines.end(),[](const auto& p){return isReagentMachine(p.second.kind);});float height=37+(hasChem?14:0)+(hasWater?14:0)+(hasWalls?14:0)+(hasFarm?14:0)+(hasPower?14:0),x=factoryView.x+10,y=factoryView.y+factoryView.height-height+5,w=factoryView.width-20;DrawRectangleRec({x-2,y-5,w+4,height},Fade(white,.93f));text("LIVE FOOD / colour = ingredient mixture",x+5,y,10,ink,true);text(fit("AIR bubbles · GEL mesh · ICE crystals · FAT droplets · HOT/COLD rim / FOOD PORTS: cyan in · dark main · orange side",w-10,8.5f),x+5,y+15,8.5f,muted);float row=y+29;if(hasPower){drawInfrastructureRoute({{x+5,row+4},{x+36,row+4}},EnergyMedium::AC,42);drawInfrastructureFitting({x+20,row+4},{1,0},EnergyMedium::AC,42);text(fit("ELECTRICITY / GOLD cable · square plugs · AC bright gold / DC amber",w-53,8.5f),x+47,row,8.5f,Color{155,104,5,255});row+=14;}if(hasWater){drawInfrastructureRoute({{x+5,row+4},{x+36,row+4}},EnergyMedium::CleanWater,42);drawInfrastructureFitting({x+20,row+4},{1,0},EnergyMedium::CleanWater,42);text(fit("WATER / BLUE pipe · round flanges · separate raw, clean and return streams",w-53,8.5f),x+47,row,8.5f,Color{23,106,181,255});row+=14;}if(hasChem){drawGlassWall({{x+5,row+4},{x+36,row+4}},5,false,false,true);text(fit("CHEMICALS / VIOLET glass · twin collars · colour inside = actual material",w-53,8.5f),x+47,row,8.5f,Color{132,47,188,255});row+=14;}if(hasFarm){text(fit("FARM / seeds → crops → harvest → wash → grade → chill → factory / stock · soil dots wash away · blue rim = chilled",w-10,8.5f),x+5,row,8.5f,Color{34,116,94,255});row+=14;}if(hasWalls)text(fit(factoryArea.empty()?"WALLS / close a loop to mark FACTORY AREA · untinted floor is outside":"FACTORY AREA / tinted floor · untinted floor is outside",w-10,8.5f,true),x+5,row,8.5f,Color{34,116,94,255},true);}

}
} // namespace aftertaste
