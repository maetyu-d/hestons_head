#include "MachineArt.h"
#include "GlassArt.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <set>
#include <stdexcept>

namespace aftertaste {
namespace {
Color hex(const std::string& s){unsigned n=unsigned(std::stoul(s.substr(1),nullptr,16));return {static_cast<unsigned char>(n>>16),static_cast<unsigned char>(n>>8),static_cast<unsigned char>(n),255};}
Color serviceColor(UtilityKind k){static const std::array<Color,5> colors={Color{66,153,203,255},Color{230,116,69,255},Color{64,166,211,255},Color{52,169,141,255},Color{150,117,188,255}};return colors.at(size_t(k));}
Color mix(Color a,Color b,float t){return {static_cast<unsigned char>(a.r*(1-t)+b.r*t),static_cast<unsigned char>(a.g*(1-t)+b.g*t),static_cast<unsigned char>(a.b*(1-t)+b.b*t),255};}
struct Art {
 Rectangle r;MachineAppearance a;Color edge,paint,pale,steel,glass,dark,hot,cold;float stroke;
 Art(Rectangle bounds,const MachineAppearance& look):r(bounds),a(look){
  if(a.kind==MachineKind::SeedHopper)a.direction=(a.direction+3)%4;
  edge={32,59,66,255};paint=hex(machineSpec(a.kind).color);if(isUtilityEquipment(a.kind))paint=serviceColor(a.utility);pale=mix(paint,WHITE,.78f);steel={169,190,191,255};glass={229,243,240,255};dark={72,99,105,255};hot={234,112,65,255};cold={62,162,204,255};
  if(a.monochrome){edge=dark=BLACK;paint=steel={184,184,184,255};pale=glass=WHITE;hot=cold={100,100,100,255};}
  if(isLabGlassware(a.kind)&&!a.monochrome)edge={68,122,137,255};
  stroke=std::clamp(std::min(r.width,r.height)*.035f,1.f,2.3f);
 }
 Color color(Color c)const{return a.ghost?Fade(c,.45f):c;}
 Vector2 p(float x,float y)const{switch((a.direction%4+4)%4){case 1:{float z=x;x=1-y;y=z;break;}case 2:x=1-x;y=1-y;break;case 3:{float z=x;x=y;y=1-z;break;}default:break;}return {r.x+x*r.width,r.y+y*r.height};}
 Rectangle rect(float x,float y,float w,float h)const{auto l=p(x,y),q=p(x+w,y+h);return {std::min(l.x,q.x),std::min(l.y,q.y),std::abs(l.x-q.x),std::abs(l.y-q.y)};}
 void line(float x,float y,float xx,float yy,Color c,float weight=1)const{DrawLineEx(p(x,y),p(xx,yy),stroke*weight,color(c));}
 void fill(float x,float y,float w,float h,Color c)const{DrawRectangleRec(rect(x,y,w,h),color(c));}
 void box(float x,float y,float w,float h,Color fill,bool rounded=true)const{auto b=rect(x,y,w,h);if(rounded){DrawRectangleRounded(b,.22f,5,color(fill));DrawRectangleRoundedLinesEx(b,.22f,5,stroke,color(edge));}else{DrawRectangleRec(b,color(fill));DrawRectangleLinesEx(b,stroke,color(edge));}}
 void oval(float x,float y,float rx,float ry,Color fill,bool outline=true)const{auto c=p(x,y);if(a.direction%2)std::swap(rx,ry);float sx=rx*r.width,sy=ry*r.height;DrawEllipse(int(c.x),int(c.y),sx,sy,color(fill));if(outline){Vector2 last{};for(int n=0;n<=32;n++){float theta=float(n)*2*PI/32;Vector2 q={c.x+std::cos(theta)*sx,c.y+std::sin(theta)*sy};if(n)DrawLineEx(last,q,stroke,color(edge));last=q;}}}
 void polygon(std::initializer_list<Vector2> vertices,Color fill)const{std::vector<Vector2> points;for(auto v:vertices)points.push_back(p(v.x,v.y));Vector2 c{};for(auto v:points){c.x+=v.x/points.size();c.y+=v.y/points.size();}for(size_t n=0;n<points.size();n++){auto x=points[n],y=points[(n+1)%points.size()];float area=(x.x-c.x)*(y.y-c.y)-(x.y-c.y)*(y.x-c.x);if(area>0)DrawTriangle(c,y,x,color(fill));else DrawTriangle(c,x,y,color(fill));DrawLineEx(x,y,stroke,color(edge));}}
 void arc(float x,float y,float rx,float ry,float first,float last,Color c,float weight=1)const{Vector2 prev{};for(int n=0;n<=20;n++){float t=first+(last-first)*n/20;auto q=p(x+std::cos(t)*rx,y+std::sin(t)*ry);if(n)DrawLineEx(prev,q,stroke*weight,color(c));prev=q;}}
 void snow(float x,float y,float s)const{for(int n=0;n<3;n++){float t=n*PI/3;line(x-std::cos(t)*s,y-std::sin(t)*s,x+std::cos(t)*s,y+std::sin(t)*s,cold,.85f);}}
 void fan(float x,float y,float s,bool rotor=true)const{oval(x,y,s,s,pale);float t=a.active&&rotor?float(a.time*3):0;for(int n=0;n<3;n++){float theta=t+n*2*PI/3;line(x,y,x+std::cos(theta)*s*.7f,y+std::sin(theta)*s*.7f,dark,1.2f);}oval(x,y,s*.15f,s*.15f,edge,false);}
 void gauge(float x,float y,float s)const{oval(x,y,s,s,glass);float t=a.active?float(std::sin(a.time*4))*.12f:0;line(x,y,x+std::cos(-.8f+t)*s*.7f,y+std::sin(-.8f+t)*s*.7f,edge,.8f);}
 void port(float x,float y,bool side=false)const{box(x-.045f,y-.075f,.09f,.15f,side?steel:paint,false);}
 void feet(float left=.20f,float right=.80f)const{box(left-.06f,.80f,.12f,.11f,steel,false);box(right-.06f,.80f,.12f,.11f,steel,false);}
};
Rectangle window(MachineKind k){switch(k){
 case MachineKind::Belt:return {.16f,.29f,.68f,.42f};
 case MachineKind::Feeder:return {.27f,.18f,.46f,.25f};
 case MachineKind::Mixer:return {.28f,.38f,.44f,.28f};
 case MachineKind::Gel:return {.24f,.34f,.43f,.24f};
 case MachineKind::Aerator:return {.35f,.25f,.32f,.39f};
 case MachineKind::Chiller:return {.20f,.32f,.40f,.35f};
 case MachineKind::Heater:return {.25f,.26f,.43f,.30f};
 case MachineKind::Dryer:return {.21f,.29f,.39f,.33f};
 case MachineKind::Cloche:return {.25f,.40f,.50f,.24f};
 case MachineKind::Cycler:return {.26f,.35f,.48f,.27f};
 case MachineKind::Dispatch:return {.31f,.37f,.38f,.26f};
 case MachineKind::Bin:return {.27f,.26f,.46f,.31f};
 case MachineKind::Doser:return {.25f,.61f,.45f,.18f};
 case MachineKind::Homogenizer:return {.57f,.34f,.26f,.28f};
 case MachineKind::Spherifier:return {.24f,.40f,.51f,.26f};
 case MachineKind::SousVide:return {.34f,.31f,.32f,.32f};
 case MachineKind::Fermenter:return {.31f,.26f,.37f,.32f};
 case MachineKind::EnzymeReactor:return {.26f,.47f,.49f,.23f};
 case MachineKind::Centrifuge:return {.38f,.33f,.24f,.30f};
 case MachineKind::Filter:return {.17f,.29f,.26f,.35f};
 case MachineKind::VacuumChamber:return {.24f,.37f,.41f,.24f};
 case MachineKind::FreezeDryer:return {.24f,.34f,.41f,.24f};
 case MachineKind::Extruder:return {.28f,.40f,.29f,.25f};
 case MachineKind::Plater:return {.32f,.39f,.36f,.28f};
 case MachineKind::Smoker:return {.28f,.36f,.40f,.29f};
 case MachineKind::Mill:return {.31f,.35f,.38f,.29f};
 case MachineKind::Binder:return {.26f,.37f,.48f,.26f};
 case MachineKind::RecipeKitchen:return {.27f,.45f,.46f,.22f};
 case MachineKind::GlassTube:case MachineKind::GlassValve:return {.20f,.43f,.60f,.14f};
 case MachineKind::GlassCoil:return {.28f,.40f,.44f,.20f};
 case MachineKind::GlassBeaker:return {.29f,.42f,.42f,.31f};
 case MachineKind::GlassFlask:return {.31f,.48f,.38f,.23f};
 case MachineKind::GlassCondenser:return {.22f,.43f,.56f,.14f};
 case MachineKind::Burette:return {.25f,.71f,.43f,.15f};
 case MachineKind::GlassJunction:return {.33f,.40f,.34f,.22f};
 case MachineKind::FractionatingStill:return {.30f,.52f,.40f,.27f};
 case MachineKind::FractionatingColumn:return {.37f,.16f,.26f,.68f};
 case MachineKind::FractionReceiver:return {.32f,.46f,.36f,.39f};
 case MachineKind::LatticeReactor:return {.27f,.37f,.46f,.30f};
 case MachineKind::Wall:return {.30f,.30f,.40f,.40f};
 case MachineKind::UtilityLine:return {.18f,.42f,.64f,.16f};
 case MachineKind::UtilityStation:return {.28f,.32f,.44f,.40f};
 case MachineKind::PHProbe:case MachineKind::TemperatureLogger:case MachineKind::DropletViewer:case MachineKind::AromaSensor:case MachineKind::SoilProbe:return {.29f,.36f,.42f,.32f};
 case MachineKind::FieldBed:case MachineKind::RaisedBed:return {.16f,.51f,.68f,.27f};
 case MachineKind::HydroponicBed:return {.15f,.48f,.70f,.28f};
 case MachineKind::SeedHopper:return {.30f,.20f,.40f,.25f};
 case MachineKind::Fertigation:return {.28f,.46f,.44f,.28f};
 case MachineKind::GrowLight:return {.28f,.43f,.44f,.28f};
 case MachineKind::PollinatorHive:return {.27f,.35f,.46f,.39f};
 case MachineKind::Harvester:return {.30f,.39f,.48f,.29f};
 case MachineKind::Washer:return {.24f,.36f,.50f,.33f};
 case MachineKind::Grader:return {.13f,.34f,.74f,.34f};
 case MachineKind::ColdStore:return {.22f,.25f,.58f,.50f};
 case MachineKind::ProduceIntake:return {.24f,.39f,.52f,.34f};
 case MachineKind::CompostBin:return {.24f,.34f,.52f,.43f};
 }return {.25f,.35f,.5f,.3f};}
}
Rectangle machineFoodWindow(Rectangle bounds,MachineKind kind,int direction,int slot,bool paired){
 auto w=window(kind);if(paired&&slot<2){w.width=(w.width-.035f)*.5f;if(slot==1)w.x+=w.width+.035f;}
 if(slot==2)w={.42f,.77f,.22f,.14f};MachineAppearance look;look.kind=kind;look.direction=direction;return Art(bounds,look).rect(w.x,w.y,w.width,w.height);
}
void drawCropProduce(Rectangle bounds,const std::string& crop,double dirt,double freshness){
 MachineAppearance look;look.kind=MachineKind::Feeder;Art d(bounds,look);d.paint=mix(hex(ingredient(crop).color),Color{108,84,48,255},float((1-freshness)*.6));Color leaf=mix(Color{66,145,76,255},Color{135,109,55,255},float(1-freshness));
 if(crop=="carrot"||crop=="beet"){
  if(crop=="carrot")d.polygon({{.24f,.36f},{.72f,.37f},{.44f,.94f}},d.paint);else d.oval(.48f,.60f,.29f,.27f,d.paint);
  for(float x:{.26f,.48f,.70f})d.line(.48f,.35f,x,.08f,leaf,1.3f);
 }else if(crop=="greens"||crop=="basil"){
  d.line(.49f,.84f,.49f,.23f,leaf,1.2f);for(float y:{.34f,.57f}){d.oval(.32f,y,.20f,.12f,leaf);d.oval(.66f,y-.06f,.20f,.12f,leaf);}d.oval(.48f,.18f,.12f,.16f,leaf);
 }else if(crop=="mushroom"){
  d.box(.39f,.43f,.22f,.42f,d.pale);d.oval(.50f,.42f,.38f,.28f,d.paint);d.line(.16f,.49f,.85f,.49f,d.edge,.6f);
 }else{
  d.oval(.48f,.58f,crop=="pepper"?.29f:.34f,.31f,d.paint);d.oval(.36f,.48f,.07f,.05f,mix(d.paint,WHITE,.5f),false);
  d.line(.48f,.28f,.53f,.10f,leaf,1.1f);d.oval(.63f,.24f,.14f,.055f,leaf,false);
  if(crop=="berry")for(float y:{.47f,.65f})for(float x:{.35f,.52f,.68f})d.oval(x,y,.018f,.026f,d.pale,false);
 }
 int dots=std::clamp(int(std::ceil(dirt*25)),0,16);for(int n=0;n<dots;n++)d.oval(.23f+float((n*37)%53)/100,.38f+float((n*23)%37)/100,.024f,.024f,Color{110,78,46,255},false);
}
void drawMachineHousing(Rectangle bounds,const MachineAppearance& look){
 Art d(bounds,look);auto k=look.kind;
 if(isGlassConduit(k)){drawGlassWall(glassPath(bounds,k,(look.direction+2)%4,look.direction),std::min(bounds.width,bounds.height)*(k==MachineKind::GlassCoil?.115f:.18f),look.ghost,look.monochrome);return;}
 // Small feet, pipes and outlines replace the identical rectangular cards.
 if(k!=MachineKind::Belt){auto shadow=d.rect(.16f,.23f,.70f,.64f);shadow.x+=1.5f;shadow.y+=2;DrawRectangleRounded(shadow,.3f,5,Fade(d.color(d.edge),.09f));}
 switch(k){
 case MachineKind::Belt:
  d.box(.02f,.15f,.96f,.70f,d.steel);d.box(.07f,.25f,.86f,.50f,d.dark,false);for(int n=0;n<5;n++)d.line(.14f+n*.18f,.28f,.14f+n*.18f,.72f,d.steel,.8f);d.line(.04f,.18f,.96f,.18f,d.paint,1.5f);d.line(.04f,.82f,.96f,.82f,d.paint,1.5f);break;
 case MachineKind::Feeder:
  d.feet(.30f,.70f);d.polygon({{.14f,.10f},{.86f,.10f},{.76f,.54f},{.58f,.73f},{.42f,.73f},{.24f,.54f}},d.pale);d.box(.12f,.07f,.76f,.12f,d.paint);d.box(.42f,.70f,.16f,.13f,d.steel,false);d.line(.50f,.82f,.90f,.82f,d.edge,1.7f);d.line(.90f,.82f,.90f,.50f,d.edge,1.7f);break;
 case MachineKind::Mixer:
  d.feet(.27f,.73f);d.box(.11f,.40f,.12f,.20f,d.paint);d.oval(.50f,.51f,.34f,.34f,d.pale);d.oval(.50f,.51f,.27f,.27f,d.glass);d.box(.39f,.03f,.22f,.16f,d.paint);d.line(.50f,.19f,.50f,.31f,d.edge,1.4f);break;
 case MachineKind::Gel:
  d.feet();d.box(.13f,.12f,.72f,.74f,d.pale);d.box(.20f,.24f,.50f,.48f,d.glass);for(float y:{.29f,.48f,.67f})d.line(.21f,y,.69f,y,d.steel,1.2f);d.box(.75f,.27f,.06f,.24f,d.paint);break;
 case MachineKind::Aerator:
  d.feet(.33f,.68f);d.oval(.50f,.22f,.25f,.14f,d.paint);d.box(.26f,.22f,.48f,.53f,d.glass);d.oval(.50f,.75f,.24f,.10f,d.pale);d.box(.03f,.58f,.17f,.23f,d.paint);d.line(.20f,.69f,.35f,.69f,d.edge,1.6f);break;
 case MachineKind::Chiller:
  d.feet();d.box(.10f,.20f,.80f,.62f,d.pale);d.box(.15f,.27f,.50f,.47f,d.glass);d.box(.11f,.12f,.77f,.13f,d.cold);d.line(.68f,.29f,.68f,.74f,d.edge);d.line(.24f,.19f,.48f,.19f,d.edge,1.1f);break;
 case MachineKind::Heater:
  d.feet();d.oval(.48f,.46f,.35f,.33f,d.paint);d.oval(.48f,.43f,.28f,.27f,d.glass);d.box(.02f,.33f,.13f,.19f,d.steel);d.box(.82f,.33f,.13f,.19f,d.steel);d.box(.18f,.76f,.61f,.13f,d.dark);break;
 case MachineKind::Dryer:
  d.feet();d.box(.12f,.12f,.57f,.73f,d.pale);d.box(.18f,.22f,.45f,.52f,d.glass);d.box(.70f,.30f,.25f,.39f,d.steel);d.line(.68f,.39f,.75f,.39f,d.edge);d.line(.68f,.59f,.75f,.59f,d.edge);break;
 case MachineKind::Cloche:
  d.box(.10f,.72f,.80f,.12f,d.paint);d.oval(.50f,.45f,.36f,.32f,d.glass);d.box(.14f,.56f,.72f,.17f,d.glass,false);d.oval(.50f,.10f,.065f,.065f,d.paint);break;
 case MachineKind::Cycler:
  d.box(.12f,.18f,.76f,.64f,d.steel);d.box(.15f,.22f,.34f,.55f,d.hot);d.box(.51f,.22f,.34f,.55f,d.cold);d.box(.22f,.31f,.56f,.36f,d.glass);d.box(.41f,.06f,.18f,.13f,d.dark);break;
 case MachineKind::Dispatch:
  d.box(.09f,.20f,.82f,.64f,d.steel);d.oval(.50f,.50f,.28f,.32f,d.pale);d.oval(.50f,.50f,.21f,.25f,d.glass);break;
 case MachineKind::Bin:
  d.polygon({{.20f,.24f},{.80f,.24f},{.72f,.87f},{.28f,.87f}},d.pale);d.box(.15f,.17f,.70f,.13f,d.dark);d.line(.20f,.12f,.75f,.04f,d.edge,1.8f);break;
 case MachineKind::Doser:
  d.box(.70f,.10f,.14f,.73f,d.steel);d.line(.38f,.21f,.75f,.21f,d.edge,1.6f);d.box(.34f,.14f,.22f,.33f,d.glass);d.box(.28f,.05f,.34f,.09f,d.paint);d.line(.45f,.47f,.45f,.59f,d.edge,1.1f);d.oval(.47f,.73f,.30f,.16f,d.pale);break;
 case MachineKind::Homogenizer:
  d.feet(.26f,.76f);d.box(.08f,.30f,.31f,.37f,d.steel);d.box(.44f,.24f,.44f,.45f,d.pale);d.box(.51f,.31f,.32f,.33f,d.glass);d.line(.38f,.47f,.48f,.47f,d.edge,2);d.gauge(.69f,.13f,.10f);break;
 case MachineKind::Spherifier:
  d.feet();d.oval(.50f,.55f,.40f,.28f,d.paint);d.oval(.50f,.52f,.34f,.22f,d.glass);d.line(.13f,.31f,.13f,.10f,d.edge,1.2f);d.line(.13f,.10f,.47f,.10f,d.edge,1.2f);d.line(.47f,.10f,.47f,.26f,d.edge,1.2f);break;
 case MachineKind::SousVide:
  d.feet();d.box(.16f,.25f,.71f,.58f,d.pale);d.box(.27f,.28f,.49f,.43f,d.glass);d.box(.08f,.06f,.20f,.49f,d.paint);d.box(.11f,.12f,.14f,.11f,d.dark);break;
 case MachineKind::Fermenter:
  d.feet(.30f,.70f);d.box(.22f,.19f,.55f,.43f,d.pale);d.oval(.50f,.19f,.28f,.10f,d.paint);d.polygon({{.23f,.58f},{.76f,.58f},{.52f,.85f}},d.pale);d.line(.50f,.85f,.50f,.92f,d.edge,1.4f);d.line(.50f,.92f,.86f,.92f,d.edge,1.2f);d.line(.86f,.92f,.86f,.5f,d.edge);break;
 case MachineKind::EnzymeReactor:
  d.feet();d.polygon({{.20f,.25f},{.50f,.10f},{.82f,.25f},{.88f,.70f},{.72f,.84f},{.23f,.84f},{.12f,.70f}},d.pale);d.box(.23f,.44f,.55f,.30f,d.glass);break;
 case MachineKind::Centrifuge:
  d.oval(.50f,.50f,.41f,.42f,d.pale);d.oval(.50f,.50f,.30f,.31f,d.glass);d.box(.38f,.02f,.24f,.12f,d.paint);break;
 case MachineKind::Filter:
  d.feet();d.box(.10f,.20f,.36f,.54f,d.glass);d.box(.54f,.15f,.33f,.62f,d.pale);d.line(.47f,.45f,.54f,.45f,d.edge,1.8f);d.line(.72f,.77f,.72f,.92f,d.edge,1.4f);d.line(.72f,.92f,.50f,.92f,d.edge,1.4f);break;
 case MachineKind::VacuumChamber:
  d.feet();d.oval(.44f,.51f,.34f,.30f,d.pale);d.oval(.44f,.51f,.27f,.23f,d.glass);d.box(.76f,.48f,.19f,.28f,d.steel);d.line(.77f,.43f,.77f,.18f,d.edge,1.4f);d.line(.77f,.18f,.62f,.18f,d.edge,1.4f);d.gauge(.46f,.12f,.11f);break;
 case MachineKind::FreezeDryer:
  d.feet();d.box(.13f,.22f,.61f,.47f,d.pale);d.oval(.21f,.45f,.10f,.23f,d.steel);d.oval(.72f,.45f,.10f,.23f,d.steel);d.box(.23f,.29f,.44f,.34f,d.glass);d.box(.73f,.74f,.20f,.16f,d.dark);d.line(.77f,.69f,.81f,.74f,d.edge);break;
 case MachineKind::Extruder:
  d.feet(.23f,.68f);d.box(.14f,.35f,.58f,.38f,d.pale);d.polygon({{.15f,.09f},{.52f,.09f},{.43f,.35f},{.25f,.35f}},d.paint);d.polygon({{.72f,.39f},{.91f,.46f},{.91f,.61f},{.72f,.69f}},d.steel);break;
 case MachineKind::Plater:
  d.oval(.50f,.54f,.32f,.33f,d.pale);d.oval(.50f,.54f,.25f,.26f,d.glass);d.box(.03f,.09f,.17f,.17f,d.paint);d.box(.80f,.09f,.17f,.17f,d.paint);break;
 case MachineKind::Smoker:
  d.feet();d.box(.19f,.23f,.59f,.57f,d.pale);d.oval(.49f,.23f,.30f,.11f,d.steel);d.box(.60f,.04f,.13f,.19f,d.dark);d.box(.56f,.02f,.22f,.07f,d.steel);d.box(.03f,.65f,.20f,.17f,d.paint);break;
 case MachineKind::Mill:
  d.feet();d.polygon({{.15f,.06f},{.85f,.06f},{.70f,.29f},{.30f,.29f}},d.paint);d.oval(.33f,.50f,.24f,.25f,d.steel);d.oval(.67f,.50f,.24f,.25f,d.steel);d.box(.27f,.73f,.46f,.13f,d.pale);break;
 case MachineKind::Binder:
  d.box(.11f,.16f,.13f,.70f,d.steel);d.box(.76f,.16f,.13f,.70f,d.steel);d.box(.15f,.19f,.70f,.13f,d.paint);d.box(.15f,.70f,.70f,.13f,d.paint);d.box(.24f,.33f,.52f,.34f,d.glass);break;
 case MachineKind::GlassTube:case MachineKind::GlassCoil:case MachineKind::GlassValve:break;
 case MachineKind::GlassBeaker:
  d.box(.18f,.18f,.64f,.66f,look.monochrome?d.glass:Fade(d.glass,.24f));d.polygon({{.18f,.18f},{.78f,.18f},{.91f,.12f},{.83f,.29f},{.83f,.81f},{.18f,.81f}},look.monochrome?d.glass:Fade(d.glass,.14f));break;
 case MachineKind::GlassFlask:
  d.box(.21f,.83f,.58f,.10f,d.steel);d.oval(.50f,.58f,.31f,.28f,look.monochrome?d.glass:Fade(d.glass,.28f));d.box(.43f,.08f,.14f,.27f,look.monochrome?d.glass:Fade(d.glass,.28f));d.box(.39f,.05f,.22f,.07f,d.pale);d.line(.57f,.23f,.85f,.23f,d.edge);d.line(.85f,.23f,.85f,.50f,d.edge);break;
 case MachineKind::GlassCondenser:
  d.box(.12f,.31f,.76f,.37f,look.monochrome?d.glass:Fade(d.cold,.16f));d.box(.08f,.41f,.84f,.18f,look.monochrome?d.glass:Fade(d.glass,.22f));d.box(.23f,.15f,.08f,.17f,d.cold);d.box(.70f,.68f,.08f,.17f,d.cold);break;
 case MachineKind::Burette:
  d.box(.79f,.06f,.07f,.80f,d.steel);d.box(.24f,.88f,.64f,.06f,d.steel);d.box(.40f,.06f,.16f,.49f,look.monochrome?d.glass:Fade(d.glass,.25f));d.line(.55f,.17f,.82f,.17f,d.edge);d.box(.38f,.55f,.20f,.09f,d.pale);d.line(.48f,.64f,.48f,.74f,d.edge);d.oval(.46f,.83f,.28f,.09f,look.monochrome?d.glass:Fade(d.glass,.25f));break;
 case MachineKind::GlassJunction:
  drawGlassWall({d.p(.02f,.50f),d.p(.50f,.50f),d.p(.98f,.50f)},std::min(bounds.width,bounds.height)*.18f,look.ghost,look.monochrome);drawGlassWall({d.p(.50f,.02f),d.p(.50f,.50f)},std::min(bounds.width,bounds.height)*.18f,look.ghost,look.monochrome);d.oval(.50f,.50f,.22f,.20f,look.monochrome?d.glass:Fade(d.glass,.25f));break;
 case MachineKind::FractionatingStill:
  d.feet(.28f,.82f);d.box(.24f,.73f,.52f,.14f,d.dark);d.oval(.50f,.59f,.34f,.34f,d.pale);d.oval(.50f,.57f,.27f,.27f,look.monochrome?d.glass:Fade(d.glass,.28f));d.box(.43f,.12f,.14f,.31f,look.monochrome?d.glass:Fade(d.glass,.24f));d.box(.37f,.08f,.26f,.08f,d.steel);d.line(.53f,.20f,.83f,.20f,d.edge,1.4f);d.line(.83f,.20f,.83f,.38f,d.edge,1.4f);break;
 case MachineKind::FractionatingColumn:
  d.feet(.31f,.84f);d.box(.33f,.11f,.34f,.77f,d.pale);d.box(.39f,.15f,.22f,.68f,look.monochrome?d.glass:Fade(d.glass,.20f));d.box(.30f,.08f,.40f,.08f,d.steel);d.box(.30f,.87f,.40f,.07f,d.steel);for(float y:{.27f,.39f,.51f,.63f,.75f})d.line(.39f,y,.61f,y,d.edge,.9f);d.line(.39f,.20f,.20f,.20f,d.edge,1.2f);d.line(.20f,.20f,.20f,.65f,d.edge,1.2f);d.line(.61f,.80f,.80f,.80f,d.edge,1.2f);break;
 case MachineKind::FractionReceiver:
  d.feet(.36f,.88f);d.line(.42f,.07f,.58f,.07f,d.steel,1.6f);d.box(.44f,.07f,.12f,.20f,look.monochrome?d.glass:Fade(d.glass,.28f));d.oval(.50f,.56f,.25f,.31f,d.pale);d.polygon({{.29f,.45f},{.42f,.25f},{.58f,.25f},{.71f,.45f},{.70f,.75f},{.59f,.88f},{.41f,.88f},{.30f,.75f}},look.monochrome?d.glass:Fade(d.glass,.22f));d.box(.42f,.89f,.16f,.07f,d.steel);break;
 case MachineKind::LatticeReactor:
  d.feet(.35f,.65);d.box(.16f,.20f,.68f,.64f,d.pale);d.box(.19f,.24f,.62f,.07f,d.paint,false);d.box(.23f,.34f,.54f,.37f,d.dark,false);d.box(.26f,.37f,.48f,.31f,look.monochrome?d.glass:Fade(d.glass,.20f),false);
  d.fill(.21f,.26f,.035f,.025f,d.pale);for(float x:{.68f,.72f,.76f})d.oval(x,.275f,.009f,.009f,d.cold,false);d.box(.30f,.78f,.40f,.06f,d.steel,false);break;
 case MachineKind::Wall:
  d.box(.02f,.12f,.96f,.76f,d.dark,false);d.box(.04f,.15f,.92f,.16f,d.steel,false);d.box(.06f,.32f,.88f,.53f,d.pale,false);d.line(.06f,.57f,.94f,.57f,d.edge,.8f);d.line(.34f,.32f,.34f,.57f,d.edge,.8f);d.line(.72f,.57f,.72f,.85f,d.edge,.8f);break;
 case MachineKind::UtilityLine:
  d.line(.06f,.50f,.94f,.50f,d.paint,2.8f);d.oval(.50f,.50f,.15f,.15f,d.pale);break;
 case MachineKind::UtilityStation:
  d.feet(.30f,.70f);d.box(.18f,.23f,.64f,.57f,d.pale);d.box(.26f,.31f,.48f,.39f,d.glass);d.gauge(.50f,.50f,.15f);d.box(.36f,.12f,.28f,.11f,d.paint);break;
 case MachineKind::PHProbe:
  d.box(.43f,.12f,.14f,.57f,d.steel);d.line(.50f,.69f,.50f,.84f,d.edge,2);d.oval(.50f,.88f,.13f,.08f,d.paint);d.box(.23f,.22f,.19f,.25f,d.pale);break;
 case MachineKind::TemperatureLogger:
  d.box(.38f,.13f,.24f,.61f,d.pale);d.box(.43f,.19f,.14f,.44f,d.glass);d.oval(.50f,.75f,.17f,.16f,d.paint);d.line(.50f,.69f,.50f,.36f,d.hot,2.4f);d.line(.27f,.24f,.73f,.24f,d.edge,.8f);break;
 case MachineKind::DropletViewer:
  d.box(.24f,.13f,.18f,.16f,d.paint);d.box(.31f,.29f,.15f,.39f,d.steel);d.box(.37f,.64f,.38f,.14f,d.pale);d.oval(.74f,.71f,.19f,.17f,d.glass);d.line(.47f,.50f,.72f,.62f,d.edge,1.5f);d.feet(.32f,.68f);break;
 case MachineKind::AromaSensor:
  d.box(.19f,.33f,.50f,.39f,d.pale);d.box(.26f,.40f,.36f,.24f,d.glass);d.line(.69f,.47f,.84f,.38f,d.paint,1.4f);d.line(.69f,.56f,.89f,.56f,d.paint,1.4f);d.line(.69f,.65f,.84f,.74f,d.paint,1.4f);d.oval(.20f,.52f,.07f,.10f,d.steel);break;
 case MachineKind::FieldBed:
  d.box(.06f,.53f,.88f,.27f,d.dark,false);d.box(.10f,.48f,.80f,.22f,d.pale,false);d.line(.12f,.65f,.88f,.65f,d.edge,.8f);d.box(.15f,.78f,.08f,.10f,d.steel,false);d.box(.77f,.78f,.08f,.10f,d.steel,false);break;
 case MachineKind::RaisedBed:
  d.box(.07f,.31f,.86f,.43f,d.paint,false);d.box(.11f,.36f,.78f,.29f,d.pale,false);d.line(.11f,.51f,.89f,.51f,d.edge,1.1f);d.box(.15f,.72f,.10f,.19f,d.steel,false);d.box(.75f,.72f,.10f,.19f,d.steel,false);d.line(.08f,.29f,.92f,.29f,d.edge,1.4f);break;
 case MachineKind::HydroponicBed:
  d.box(.06f,.34f,.88f,.42f,d.steel,false);d.box(.10f,.42f,.80f,.29f,look.monochrome?d.glass:Fade(d.cold,.20f),false);d.line(.11f,.42f,.88f,.42f,d.cold,1.2f);d.line(.17f,.72f,.83f,.72f,d.edge,1.5f);for(float x:{.28f,.50f,.72f}){d.line(x,.30f,x,.54f,d.edge,1.4f);d.oval(x,.29f,.055f,.07f,d.paint);}break;
 case MachineKind::SeedHopper:
  d.feet(.25f,.75f);d.box(.19f,.12f,.62f,.38f,d.pale);d.box(.25f,.06f,.50f,.10f,d.paint);d.polygon({{.22f,.51f},{.78f,.51f},{.58f,.75f},{.42f,.75f}},d.steel);d.oval(.50f,.29f,.08f,.07f,d.paint);d.line(.50f,.74f,.50f,.91f,d.edge,1.5f);break;
 case MachineKind::Fertigation:
  d.feet(.27f,.73f);d.oval(.48f,.58f,.30f,.29f,d.pale);d.oval(.48f,.58f,.24f,.22f,d.glass);d.box(.20f,.20f,.55f,.10f,d.paint);d.line(.48f,.19f,.48f,.32f,d.edge,1.4f);d.line(.19f,.34f,.19f,.54f,d.edge,1.3f);d.line(.19f,.34f,.32f,.34f,d.edge,1.3f);d.gauge(.78f,.38f,.08f);break;
 case MachineKind::GrowLight:
  d.box(.18f,.14f,.64f,.14f,d.steel,false);d.box(.24f,.26f,.52f,.18f,d.paint);d.line(.28f,.44f,.18f,.72f,d.edge,1.4f);d.line(.50f,.44f,.50f,.74f,d.edge,1.4f);d.line(.72f,.44f,.82f,.72f,d.edge,1.4f);d.box(.11f,.73f,.78f,.10f,d.dark,false);break;
 case MachineKind::PollinatorHive:
  d.feet(.30f,.70f);d.box(.19f,.29f,.62f,.52f,d.pale,false);d.box(.15f,.23f,.70f,.10f,d.paint,false);d.box(.22f,.39f,.56f,.12f,d.steel,false);d.box(.22f,.56f,.56f,.12f,d.steel,false);d.oval(.50f,.76f,.07f,.035f,d.edge,false);d.line(.32f,.84f,.68f,.84f,d.edge,1.2f);break;
 case MachineKind::SoilProbe:
  d.box(.14f,.21f,.50f,.38f,d.pale);d.box(.21f,.28f,.36f,.23f,d.glass);d.line(.66f,.40f,.86f,.40f,d.edge,1.6f);d.line(.84f,.40f,.84f,.87f,d.edge,1.8f);d.line(.77f,.87f,.91f,.87f,d.paint,1.5f);d.gauge(.38f,.39f,.07f);break;
 case MachineKind::Harvester:
  d.feet(.28f,.75f);d.box(.21f,.24f,.57f,.50f,d.pale);d.box(.29f,.33f,.38f,.29f,d.glass);d.box(.26f,.15f,.48f,.10f,d.paint);d.box(.07f,.63f,.30f,.12f,d.steel);d.line(.07f,.63f,.03f,.83f,d.edge,1.5f);d.line(.37f,.69f,.11f,.87f,d.edge,1.2f);d.oval(.29f,.85f,.10f,.07f,d.dark);d.oval(.73f,.85f,.09f,.065f,d.dark);break;
 case MachineKind::Washer:
  d.feet(.24f,.76f);d.box(.10f,.25f,.80f,.57f,d.pale);d.oval(.51f,.53f,.29f,.27f,d.glass);d.oval(.51f,.53f,.20f,.18f,d.cold);d.box(.12f,.16f,.76f,.11f,d.paint);d.line(.22f,.14f,.22f,.04f,d.cold,1.3f);d.line(.50f,.14f,.50f,.04f,d.cold,1.3f);d.line(.78f,.14f,.78f,.04f,d.cold,1.3f);break;
 case MachineKind::Grader:
  d.box(.05f,.28f,.90f,.49f,d.steel,false);d.box(.09f,.35f,.82f,.30f,d.dark,false);for(float x:{.20f,.38f,.58f,.78f})d.oval(x,.50f,.075f,.13f,d.pale);d.line(.10f,.69f,.90f,.69f,d.paint,1.5f);d.feet(.20f,.80f);break;
 case MachineKind::ColdStore:
  d.feet(.23f,.77f);d.box(.10f,.10f,.80f,.77f,d.pale,false);d.box(.16f,.17f,.48f,.62f,d.glass,false);d.box(.68f,.17f,.16f,.62f,d.steel,false);d.line(.72f,.46f,.79f,.46f,d.edge,1.6f);d.snow(.41f,.31f,.09f);d.snow(.41f,.59f,.09f);d.box(.17f,.04f,.65f,.08f,d.cold,false);break;
 case MachineKind::ProduceIntake:
  d.feet(.25f,.75f);d.box(.13f,.27f,.74f,.54f,d.pale,false);d.box(.20f,.35f,.60f,.36f,d.glass,false);d.line(.20f,.47f,.80f,.47f,d.edge,.8f);d.line(.20f,.59f,.80f,.59f,d.edge,.8f);d.line(.40f,.35f,.40f,.71f,d.edge,.8f);d.line(.60f,.35f,.60f,.71f,d.edge,.8f);d.box(.27f,.12f,.46f,.10f,d.paint);break;
 case MachineKind::CompostBin:
  d.feet(.25f,.75f);d.box(.13f,.23f,.74f,.61f,d.pale);d.box(.20f,.31f,.60f,.42f,d.dark);d.oval(.50f,.52f,.22f,.17f,d.glass);d.line(.28f,.52f,.72f,.52f,d.edge,1.3f);d.line(.40f,.39f,.60f,.65f,d.paint,1.4f);d.line(.60f,.39f,.40f,.65f,d.paint,1.4f);d.box(.23f,.13f,.54f,.08f,d.steel);break;
 case MachineKind::RecipeKitchen:
  d.feet();d.box(.12f,.16f,.76f,.68f,d.pale);d.box(.22f,.42f,.56f,.31f,d.dark);for(float x:{.30f,.70f})d.oval(x,.28f,.10f,.08f,d.steel);break;
 }
 // These are routing hardware, not extra inventory or simulated ingredient flow.
 if(k!=MachineKind::Belt&&k!=MachineKind::Wall&&!isUtilityEquipment(k)&&!isSensor(k)&&!isFarmInfrastructure(k)){if(k==MachineKind::LatticeReactor){d.box(.07f,.465f,.12f,.07f,d.steel,false);d.box(.81f,.465f,.12f,.07f,d.steel,false);d.line(.08f,.50f,.18f,.50f,d.glass,.65f);d.line(.82f,.50f,.92f,.50f,d.glass,.65f);}else{if(k!=MachineKind::Bin)d.port(.94f,.50f);if(k!=MachineKind::Feeder)d.port(.06f,.50f);}if(k==MachineKind::Mixer||k==MachineKind::Plater)d.port(.50f,.055f);if(hasSideOutlet(k))d.port(.50f,.94f,true);}
}
void drawMachineMechanism(Rectangle bounds,const MachineAppearance& look){
 Art d(bounds,look);float t=look.active?float(look.time):0;bool fine=std::min(bounds.width,bounds.height)>=29;
 switch(look.kind){
 case MachineKind::Belt:for(float x:{.27f,.52f,.77f}){d.line(x-.07f,.39f,x+.03f,.50f,d.glass,.9f);d.line(x+.03f,.50f,x-.07f,.61f,d.glass,.9f);}break;
 case MachineKind::Feeder:d.line(.29f,.49f,.43f,.63f,d.paint,1.2f);d.line(.71f,.49f,.57f,.63f,d.paint,1.2f);d.line(.37f,.77f,.63f,.77f,d.edge);break;
 case MachineKind::Mixer:{float theta=t*2;for(int n=0;n<3;n++){float a=theta+n*2*PI/3;d.line(.50f,.51f,.50f+std::cos(a)*.23f,.51f+std::sin(a)*.23f,d.edge,1.5f);}d.oval(.50f,.51f,.05f,.05f,d.paint);break;}
 case MachineKind::Gel:for(float y:{.28f,.60f,.72f})d.line(.22f,y,.69f,y,d.paint,1.3f);if(fine){d.line(.29f,.17f,.41f,.17f,d.paint);d.line(.32f,.13f,.32f,.21f,d.paint);}break;
 case MachineKind::Aerator:d.line(.35f,.72f,.66f,.72f,d.edge);for(float x:{.38f,.50f,.62f}){d.line(x,.73f,x,.66f,d.paint,1.2f);d.oval(x,.63f,.025f,.025f,d.glass);}d.gauge(.11f,.68f,.06f);break;
 case MachineKind::Chiller:d.snow(.78f,.47f,.11f);for(float y:{.65f,.71f,.77f})d.line(.73f,y,.85f,y,d.cold,.7f);break;
 case MachineKind::Heater:for(int n=0;n<4;n++){float x=.26f+n*.13f;d.line(x,.86f,x+.055f,.79f,d.hot,1.4f);d.line(x+.055f,.79f,x+.11f,.86f,d.hot,1.4f);}break;
 case MachineKind::Dryer:d.fan(.82f,.50f,.10f);for(float y:{.25f,.46f,.67f})d.line(.19f,y,.62f,y,d.edge,.7f);break;
 case MachineKind::Cloche:d.arc(.50f,.45f,.30f,.26f,PI,1.6f*PI,d.steel,.7f);d.line(.15f,.72f,.85f,.72f,d.edge,1.3f);if(fine)d.arc(.51f,.31f,.075f,.07f,-PI*.6f,PI*.5f,d.paint,.8f);break;
 case MachineKind::Cycler:d.arc(.50f,.49f,.35f,.34f,PI*.9f,PI*1.85f,d.edge);d.line(.80f,.24f,.83f,.40f,d.edge);d.snow(.77f,.71f,.065f);for(float x:{.19f,.27f,.35f}){d.line(x,.74f,x+.035f,.67f,d.hot,.8f);d.line(x+.035f,.67f,x+.07f,.74f,d.hot,.8f);}break;
 case MachineKind::Dispatch:d.line(.17f,.35f,.17f,.73f,d.edge);for(float x:{.13f,.17f,.21f})d.line(x,.25f,x,.42f,d.edge,.65f);d.oval(.84f,.33f,.055f,.095f,d.steel);d.line(.84f,.42f,.84f,.73f,d.edge);break;
 case MachineKind::Bin:for(float x:{.34f,.50f,.66f})d.line(x,.63f,x,.80f,d.edge,.8f);d.line(.38f,.37f,.50f,.49f,d.edge,1.2f);d.line(.50f,.49f,.62f,.37f,d.edge,1.2f);break;
 case MachineKind::Doser:for(float y:{.23f,.31f,.39f})d.line(.37f,y,.45f,y,d.paint,.7f);d.line(.45f,.02f,.45f,.13f,d.edge,1.1f);if(look.active)d.oval(.45f,.55f,.025f,.035f,d.paint);break;
 case MachineKind::Homogenizer:{float shift=look.active?std::sin(t*5)*.045f:0;d.box(.14f+shift,.35f,.11f,.27f,d.dark,false);d.line(.25f+shift,.48f,.49f,.48f,d.edge,1.5f);d.line(.49f,.30f,.49f,.65f,d.edge,1.6f);break;}
 case MachineKind::Spherifier:d.oval(.47f,.29f,.13f,.08f,d.steel);for(float x:{.41f,.47f,.53f})d.oval(x,.29f,.013f,.013f,d.edge,false);break;
 case MachineKind::SousVide:d.box(.30f,.28f,.40f,.39f,Fade(d.glass,.08f));d.line(.32f,.33f,.68f,.33f,d.paint,.8f);d.line(.17f,.29f,.17f,.47f,d.edge);d.oval(.17f,.46f,.035f,.04f,d.hot);break;
 case MachineKind::Fermenter:d.line(.50f,.12f,.50f,.035f,d.edge);d.line(.50f,.035f,.65f,.035f,d.edge);d.line(.65f,.035f,.65f,.12f,d.edge);d.oval(.65f,.10f,.045f,.045f,d.glass);break;
 case MachineKind::EnzymeReactor:d.oval(.36f,.29f,.065f,.065f,d.glass);d.oval(.59f,.29f,.065f,.065f,d.glass);d.line(.40f,.33f,.62f,.44f,d.edge,1.1f);d.line(.55f,.33f,.35f,.44f,d.edge,1.1f);break;
 case MachineKind::Centrifuge:{float theta=t*2.8f;for(int n=0;n<4;n++){float a=theta+n*PI*.5f;float x=.50f+std::cos(a)*.25f,y=.50f+std::sin(a)*.25f;d.line(.50f,.50f,x,y,d.edge,1.1f);d.oval(x,y,.055f,.08f,d.paint);}d.oval(.50f,.50f,.06f,.06f,d.steel);break;}
 case MachineKind::Filter:for(float y:{.25f,.36f,.47f,.58f,.69f})d.line(.59f,y,.81f,y,d.edge,.75f);for(float x:{.63f,.71f,.79f})d.line(x,.23f,x,.71f,d.paint,.65f);break;
 case MachineKind::VacuumChamber:for(float x:{.25f,.43f,.62f}){d.line(x,.25f,x,.31f,d.steel,1.3f);d.line(x,.71f,x,.77f,d.steel,1.3f);}d.gauge(.85f,.61f,.055f);break;
 case MachineKind::FreezeDryer:d.snow(.43f,.13f,.11f);for(int n=0;n<3;n++)d.arc(.43f,.77f+n*.04f,.22f,.03f,0,PI,d.edge,.65f);break;
 case MachineKind::Extruder:for(int n=0;n<4;n++){float x=.23f+n*.11f;d.line(x,.39f,x+.08f,.67f,d.edge,.85f);}for(float y:{.46f,.53f,.60f})d.line(.85f,y,.93f,y,d.edge,.7f);break;
 case MachineKind::Plater:{float reach=look.active?std::sin(t*2)*.04f:0;d.line(.12f,.18f,.24f,.38f,d.edge,1.6f);d.line(.24f,.38f,.34f+reach,.43f,d.paint,1.5f);d.line(.88f,.18f,.76f,.38f,d.edge,1.6f);d.line(.76f,.38f,.66f-reach,.43f,d.paint,1.5f);d.oval(.24f,.38f,.04f,.04f,d.steel);d.oval(.76f,.38f,.04f,.04f,d.steel);break;}
 case MachineKind::Smoker:for(float y:{.34f,.68f})d.line(.25f,y,.72f,y,d.edge,.8f);d.line(.79f,.37f,.79f,.62f,d.edge);if(look.active)d.arc(.66f,.025f,.04f,.08f,PI*.8f,PI*1.8f,d.dark,.8f);break;
 case MachineKind::Mill:for(int side=0;side<2;side++){float x=side?.67f:.33f,a=(side?-1:1)*t*2;for(int n=0;n<5;n++){float theta=a+n*2*PI/5;d.line(x+std::cos(theta)*.15f,.50f+std::sin(theta)*.15f,x+std::cos(theta)*.22f,.50f+std::sin(theta)*.22f,d.edge,1.1f);}d.oval(x,.50f,.055f,.055f,d.paint);}break;
 case MachineKind::Binder:d.line(.50f,.12f,.50f,.31f,d.edge,1.3f);d.line(.35f,.31f,.65f,.31f,d.edge,1.1f);d.line(.35f,.68f,.65f,.68f,d.edge,1.1f);d.oval(.37f,.14f,.08f,.045f,d.glass);d.oval(.50f,.14f,.08f,.045f,d.glass);d.oval(.63f,.14f,.08f,.045f,d.glass);break;
 case MachineKind::GlassTube:d.line(.77f,.45f,.84f,.50f,d.edge,.55f);d.line(.84f,.50f,.77f,.55f,d.edge,.55f);break;
 case MachineKind::GlassCoil:d.line(.17f,.12f,.17f,.88f,d.steel,.6f);d.line(.84f,.12f,.84f,.88f,d.steel,.6f);break;
 case MachineKind::GlassValve:
  d.oval(.50f,.50f,.11f,.11f,look.monochrome?d.glass:Fade(d.glass,.18f));d.line(.50f,.31f,.50f,.45f,d.edge,1.3f);if(look.closed)d.line(.50f,.13f,.50f,.37f,d.paint,2);else d.line(.29f,.26f,.71f,.26f,d.paint,2);break;
 case MachineKind::GlassBeaker:
  for(float y:{.29f,.39f,.49f,.59f,.69f})d.line(.22f,y,.30f,y,d.edge,.55f);d.line(.76f,.31f,.76f,.72f,WHITE,.65f);d.arc(.50f,.77f,.24f,.03f,0,PI,d.steel,.55f);break;
 case MachineKind::GlassFlask:
  d.arc(.50f,.58f,.26f,.23f,PI,PI*1.55f,WHITE,.7f);d.line(.46f,.14f,.46f,.30f,WHITE,.6f);for(float x:{.30f,.43f,.56f}){d.line(x,.90f,x+.05f,.85f,d.hot,.9f);d.line(x+.05f,.85f,x+.10f,.90f,d.hot,.9f);}break;
 case MachineKind::GlassCondenser:
  d.line(.18f,.35f,.82f,.35f,WHITE,.7f);for(float x:{.20f,.80f}){d.line(x,.29f,x,.40f,d.edge,.7f);d.line(x,.60f,x,.70f,d.edge,.7f);}d.snow(.50f,.23f,.065f);break;
 case MachineKind::Burette:
  for(float y:{.15f,.23f,.31f,.39f,.47f})d.line(.42f,y,.49f,y,d.edge,.5f);d.line(.32f,.59f,.64f,.59f,d.paint,1.6f);d.line(.52f,.12f,.52f,.48f,WHITE,.6f);break;
 case MachineKind::GlassJunction:
  d.arc(.50f,.50f,.17f,.15f,PI,PI*1.6f,WHITE,.65f);d.line(.39f,.50f,.61f,.50f,d.paint,.65f);break;
 case MachineKind::FractionatingStill:
  if(look.active){float bubble=.5f+.16f*std::sin(t*5);d.oval(.43f,bubble,.025f,.035f,d.glass);d.oval(.56f,bubble-.12f,.02f,.028f,d.glass);}for(float x:{.37f,.46f,.55f,.64f}){d.line(x,.84f,x+.045f,.78f,d.hot,.9f);d.line(x+.045f,.78f,x+.09f,.84f,d.hot,.9f);}d.line(.44f,.42f,.56f,.42f,d.paint,.8f);break;
 case MachineKind::FractionatingColumn:
  for(float y:{.25f,.37f,.49f,.61f,.73f}){d.line(.38f,y,.62f,y,d.edge,.8f);d.oval(.50f,y,.018f,.018f,d.glass);}if(look.active){float y=.77f-float(std::fmod(t*.35,1.))*.52f;d.oval(.50f,y,.025f,.035f,d.paint);}break;
 case MachineKind::FractionReceiver:
  for(float y:{.48f,.58f,.68f,.78f})d.line(.38f,y,.45f,y,d.edge,.55f);d.line(.72f,.44f,.72f,.73f,WHITE,.65f);d.line(.50f,.85f,.50f,.95f,d.edge,1);d.line(.39f,.95f,.61f,.95f,d.edge,1);break;
 case MachineKind::LatticeReactor:
  for(int y=0;y<5;y++)for(int x=0;x<5;x++){
   float px=.32f+x*.09f,py=.415f+y*.055f;
   if(x<4)d.line(px,py,px+.09f,py,d.edge,.45f);
   if(y<4)d.line(px,py,px,py+.055f,d.edge,.45f);
   if(x<4&&y<4&&(x+y)%2==0)d.line(px,py,px+.09f,py+.055f,d.steel,.35f);
  }
  for(int y=0;y<5;y++)for(int x=0;x<5;x++){
   float px=.32f+x*.09f,py=.415f+y*.055f;Color c=((x+y)%4==0)?d.paint:((x+y)%4==1)?d.cold:d.pale;d.oval(px,py,.018f,.018f,c);
  }
  if(look.active){int cell=int(std::fmod(t*8,25));float x=.32f+(cell%5)*.09f,y=.415f+(cell/5)*.055f;float l=x-.03f,r=x+.03f,top=y-.023f,bottom=y+.023f;d.line(l,top,r,top,d.hot,.9f);d.line(r,top,r,bottom,d.hot,.9f);d.line(r,bottom,l,bottom,d.hot,.9f);d.line(l,bottom,l,top,d.hot,.9f);}
  break;
 case MachineKind::UtilityLine:for(float x:{.30f,.50f,.70f})d.oval(x,.50f,.035f,.035f,d.pale,false);break;
 case MachineKind::UtilityStation:d.gauge(.50f,.50f,.15f);for(float x:{.30f,.70f})d.line(x,.22f,x,.30f,d.paint,1.1f);break;
 case MachineKind::PHProbe:for(float y:{.28f,.38f,.48f,.58f})d.line(.61f,y,.72f,y,d.edge,.7f);d.line(.50f,.18f,.50f,.82f,d.paint,1.2f);break;
 case MachineKind::TemperatureLogger:for(float y:{.27f,.38f,.49f,.60f})d.line(.58f,y,.68f,y,d.edge,.7f);if(look.active){float level=.4f+.18f*std::sin(t*2);d.line(.50f,.68f,.50f,level,d.hot,2);}break;
 case MachineKind::DropletViewer:for(int n=0;n<7;n++){float x=.28f+.13f*(n%4),y=.30f+.16f*(n/4);d.oval(x,y,.035f,.035f,n%2?d.cold:d.paint,false);}break;
 case MachineKind::AromaSensor:for(float k=0;k<3;k++){float spread=.04f+k*.06f;d.arc(.74f,.52f,spread,.13f+k*.04f,-PI*.36f,PI*.36f,d.paint,.7f);}break;
 case MachineKind::FieldBed:case MachineKind::RaisedBed:case MachineKind::HydroponicBed:{
  Color leaf=mix(Color{61,145,79,255},Color{160,118,61,255},float(1-look.cropHealth));if(look.monochrome)leaf=d.paint;
  d.line(.17f,.76f,.17f+.66f*float(look.cropMoisture),.76f,d.cold,1.3f);
  if(look.cropPlanted){float growth=float(std::clamp(look.progress,0.,1.));
   for(int n=0;n<4;n++){float x=.23f+n*.18f,h=.05f+growth*.28f;
    if(look.crop=="mushroom"){d.box(x-.02f,.65f-h*.65f,.04f,h*.65f,d.pale,false);d.oval(x,.65f-h*.7f,.025f+growth*.05f,.02f+growth*.04f,d.paint);continue;}
    d.line(x,.67f,x,.67f-h,leaf,1.1f);if(growth>.10){d.oval(x-.035f,.67f-h*.65f,.045f,.025f,leaf,false);d.oval(x+.035f,.67f-h*.90f,.045f,.026f,leaf,false);}
    if(growth>.55&&look.crop!="greens"&&look.crop!="basil"){Color fruit=look.monochrome?d.paint:mix(leaf,hex(ingredient(look.crop).color),(growth-.55f)/.45f);float y=look.crop=="carrot"||look.crop=="beet"?.68f:.67f-h*.60f;d.oval(x+.025f,y,.02f+growth*.017f,.025f+growth*.02f,fruit,false);}
   }
  }else for(float x:{.24f,.50f,.76f})d.line(x,.54f,x+.04f,.58f,d.steel,.7f);
  break;}
 case MachineKind::SeedHopper:for(float x:{.40f,.50f,.60f}){d.oval(x,.28f,.025f,.023f,d.paint,false);d.line(x,.33f,x,.39f,d.edge,.65f);}break;
 case MachineKind::Fertigation:{float a=look.active?t*2:0;for(int n=0;n<3;n++){float x=.40f+std::cos(a+n*2*PI/3)*.08f,y=.57f+std::sin(a+n*2*PI/3)*.07f;d.oval(x,y,.035f,.035f,d.paint,false);}d.line(.17f,.37f,.30f,.37f,d.cold,1.2f);break;}
 case MachineKind::GrowLight:for(float x:{.30f,.50f,.70f}){float y=.49f+(look.active?.09f*std::sin(t*3+x*4):.09f);d.line(x,.45f,x-.06f,y,d.paint,.8f);d.line(x,.45f,x+.06f,y,d.paint,.8f);}break;
 case MachineKind::PollinatorHive:for(float y:{.46f,.61f})for(float x:{.34f,.50f,.66f}){d.oval(x,y,.035f,.03f,d.paint,false);d.line(x-.03f,y,x+.03f,y,d.edge,.5f);}if(look.active){d.oval(.86f,.22f,.035f,.025f,d.hot,false);d.line(.82f,.22f,.78f,.20f,d.edge,.7f);}break;
 case MachineKind::SoilProbe:for(float y:{.34f,.43f,.52f}){float v=look.progress;d.line(.29f,y,.29f+.12f*(.35f+v),y,d.paint,1.2f);}break;
 case MachineKind::Harvester:for(float n=0;n<5;n++){float x=.10f+n*.06f;d.line(x,.66f,x+.04f,.79f,d.edge,.8f);}if(look.active)d.line(.13f,.79f,.42f,.61f,d.hot,1.5f);break;
 case MachineKind::Washer:{float a=look.active?t*2:0;d.line(.33f,.50f,.69f,.50f,d.edge,1.1f);for(int n=0;n<3;n++){float angle=a+n*2*PI/3;d.line(.51f,.53f,.51f+std::cos(angle)*.14f,.53f+std::sin(angle)*.13f,d.cold,1.2f);}for(float x:{.23f,.50f,.77f}){d.line(x,.18f,x,.26f,d.cold,.8f);d.oval(x,.28f,.018f,.025f,d.cold,false);}break;}
 case MachineKind::Grader:for(float x:{.20f,.38f,.58f,.78f})d.line(x,.38f,x,.62f,d.paint,.8f);d.line(.14f,.72f,.88f,.72f,d.edge,1);break;
 case MachineKind::ColdStore:d.snow(.76f,.38f,.07f);d.snow(.76f,.62f,.07f);d.line(.22f,.73f,.58f,.73f,d.cold,.8f);break;
 case MachineKind::ProduceIntake:for(float x:{.29f,.43f,.57f,.71f}){d.box(x-.035f,.38f,.07f,.06f,d.paint,false);d.box(x-.035f,.51f,.07f,.06f,d.paint,false);d.box(x-.035f,.63f,.07f,.06f,d.paint,false);}break;
 case MachineKind::CompostBin:{float angle=look.active?t*2:0;for(int n=0;n<3;n++){float a=angle+n*2*PI/3;d.line(.50f,.52f,.50f+std::cos(a)*.14f,.52f+std::sin(a)*.11f,d.paint,1.3f);}d.oval(.50f,.52f,.035f,.035f,d.edge,false);break;}
 case MachineKind::Wall:break;
 case MachineKind::RecipeKitchen:d.line(.30f,.49f,.70f,.49f,d.steel);d.line(.30f,.67f,.70f,.67f,d.steel);break;
 }
 // A physical outlet arrow stays readable while the machine itself is rotated.
 if(look.kind!=MachineKind::Bin&&look.kind!=MachineKind::Wall&&!isUtilityEquipment(look.kind)&&!isFarmInfrastructure(look.kind)){d.line(.89f,.45f,.96f,.50f,d.edge,.8f);d.line(.96f,.50f,.89f,.55f,d.edge,.8f);}
}
int verifyMachineArtwork(){
 int checked=0;
 for(int size:{24,64})for(int direction=0;direction<4;direction++){
  std::set<uint64_t> pictures;auto target=LoadRenderTexture(size,size);
  auto picture=[&](MachineAppearance look){
   BeginTextureMode(target);ClearBackground(WHITE);drawMachineHousing({2,2,float(size-4),float(size-4)},look);drawMachineMechanism({2,2,float(size-4),float(size-4)},look);EndTextureMode();
   auto pixels=LoadImageFromTexture(target.texture);auto colors=LoadImageColors(pixels);uint64_t hash=1469598103934665603ull;
   for(int n=0;n<size*size;n++){if(colors[n].r!=colors[n].g||colors[n].g!=colors[n].b){UnloadImageColors(colors);UnloadImage(pixels);throw std::runtime_error("Machine silhouette QA must be independent of paint colour");}hash=(hash^colors[n].r)*1099511628211ull;}
   UnloadImageColors(colors);UnloadImage(pixels);return hash;
  };
  for(auto& spec:machineSpecs()){
   MachineAppearance look;look.kind=spec.kind;look.direction=direction;look.monochrome=true;
   auto hash=picture(look);if(!pictures.insert(hash).second)throw std::runtime_error("Machine artwork is not distinct without colour: "+spec.name);
   look.time=.73;if(picture(look)!=hash)throw std::runtime_error("Idle machinery should not animate: "+spec.name);
   look.ghost=true;if(picture(look)==hash)throw std::runtime_error("Placement previews should remain translucent: "+spec.name);look.ghost=false;
   for(int slot=0;slot<3;slot++){auto r=machineFoodWindow({0,0,float(size),float(size)},spec.kind,direction,slot,slot<2);if(r.x<0||r.y<0||r.width<=0||r.height<=0||r.x+r.width>size+.01f||r.y+r.height>size+.01f)throw std::runtime_error("Food window leaves its physical machine: "+spec.name);}
   if(size==64&&(spec.kind==MachineKind::Mixer||spec.kind==MachineKind::Homogenizer||spec.kind==MachineKind::Centrifuge||spec.kind==MachineKind::Mill)){look.active=true;look.time=0;auto still=picture(look);look.time=.37;if(picture(look)==still)throw std::runtime_error("Working mechanism must respond to activity: "+spec.name);}
   checked++;
  }
  UnloadRenderTexture(target);
 }
 return checked;
}
} // namespace aftertaste
