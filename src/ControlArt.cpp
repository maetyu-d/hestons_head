#include "ControlArt.h"
#include "Control.h"
#include "Theme.h"
#include <algorithm>
#include <cmath>
namespace aftertaste {
namespace {
struct Drawing{
 Rectangle r;const MachineAppearance& a;
 Vector2 p(float x,float y)const{for(int n=0;n<a.direction;n++){float old=x;x=1-y;y=old;}return {r.x+x*r.width,r.y+y*r.height};}
 Color color(Color c)const{return a.ghost?Fade(c,.4f):c;}
 void line(float x,float y,float xx,float yy,Color c,float w=2)const{DrawLineEx(p(x,y),p(xx,yy),std::max(.7f,r.width*w/60),color(c));}
 void circle(float x,float y,float rad,Color c)const{DrawCircleV(p(x,y),rad*std::min(r.width,r.height),color(c));}
 void box(float x,float y,float w,float h,Color fill)const{auto b=p(x,y),e=p(x+w,y+h);Rectangle q{std::min(b.x,e.x),std::min(b.y,e.y),std::abs(e.x-b.x),std::abs(e.y-b.y)};DrawRectangleRec(q,color(fill));DrawRectangleLinesEx(q,std::max(.7f,r.width/60),color(theme::Ink));}
};
}
bool drawControlHousing(Rectangle r,const MachineAppearance& a){
 if(a.kind==MachineKind::SignalLead){Drawing dots{r,a};for(float x=.1f;x<1;x+=.16f)dots.line(x,.5f,x+.07f,.5f,theme::Ink,1.7f);dots.circle(.5f,.5f,.08f,theme::Blue);return true;}
 if(!isControlDevice(a.kind))return false;Drawing d{r,a};using K=MachineKind;Color ink=theme::Ink,white=WHITE,blue=a.monochrome?ink:theme::Blue,green=a.monochrome?ink:theme::Green,yellow=a.monochrome?ink:theme::Yellow;
 if(a.kind==K::WaterDiverter){d.line(.04f,.5f,.96f,.5f,blue,8);d.line(.5f,.5f,.5f,.96f,blue,8);for(auto p:std::vector<Vector2>{{.07f,.5f},{.93f,.5f},{.5f,.93f}}){d.circle(p.x,p.y,.065f,ink);d.circle(p.x,p.y,.04f,white);}d.circle(.5f,.5f,.22f,ink);d.circle(.5f,.5f,.17f,white);d.box(.26f,.12f,.48f,.15f,blue);return true;}
 if(a.kind==K::ProcessSensor){d.box(.08f,.22f,.5f,.56f,theme::blend(blue,white,.5f));d.box(.17f,.31f,.31f,.20f,ink);d.line(.58f,.5f,.93f,.5f,ink,3);d.circle(.91f,.5f,.055f,blue);d.line(.02f,.5f,.08f,.5f,ink);return true;}
 if(a.kind==K::ControlTimer){d.box(.12f,.1f,.76f,.8f,theme::blend(blue,white,.6f));d.circle(.5f,.44f,.25f,ink);d.circle(.5f,.44f,.21f,white);for(int i=0;i<4;i++){float ang=float(i)*1.5708f;d.line(.5f+.16f*std::cos(ang),.44f+.16f*std::sin(ang),.5f+.20f*std::cos(ang),.44f+.20f*std::sin(ang),ink,1);}d.line(.02f,.5f,.12f,.5f,ink);d.line(.88f,.5f,.98f,.5f,ink);return true;}
 if(a.kind==K::ControlActuator){d.box(.17f,.27f,.46f,.46f,theme::blend(green,white,.35f));for(float x:{.25f,.35f,.45f,.55f})d.line(x,.31f,x,.69f,ink,1);d.line(.03f,.5f,.17f,.5f,ink);d.line(.63f,.5f,.96f,.5f,ink,3);d.line(.83f,.37f,.96f,.5f,ink,2);d.line(.83f,.63f,.96f,.5f,ink,2);return true;}
 d.box(.13f,.15f,.74f,.7f,theme::blend(yellow,white,.25f));d.line(.02f,.5f,.13f,.5f,ink);d.line(.87f,.5f,.98f,.5f,ink);
 if(a.kind==K::LogicGate){d.line(.5f,.02f,.5f,.15f,ink);d.line(.25f,.35f,.48f,.5f,ink);d.line(.25f,.65f,.48f,.5f,ink);d.line(.48f,.5f,.75f,.5f,ink,3);d.circle(.7f,.5f,.1f,white);}
 else{d.box(.23f,.29f,.53f,.19f,ink);d.line(.26f,.68f,.72f,.68f,ink,1);d.line(.4f,.58f,.4f,.76f,blue,2);d.line(.6f,.58f,.6f,.76f,green,2);}
 return true;
}
void drawControlMechanism(Rectangle r,const MachineAppearance& a){if(!isControlDevice(a.kind))return;Drawing d{r,a};Color state=a.controlFault?theme::Red:a.controlValid?(a.controlSignal?theme::Green:theme::Muted):Color{190,201,211,255};if(a.monochrome)state=theme::Ink;
 d.circle(.28f,.72f,.045f,state);
 if(a.kind==MachineKind::ControlTimer){float angle=float(a.progress)*6.28318f-1.5708f;d.line(.5f,.44f,.5f+.16f*std::cos(angle),.44f+.16f*std::sin(angle),theme::Ink,2);}
 if(a.kind==MachineKind::WaterDiverter){bool side=a.controlFault||a.controlSignal;d.line(.5f,.5f,side?.5f:.78f,side?.78f:.5f,side?theme::Red:theme::Green,3);}
 if(a.closed){d.line(.24f,.25f,.76f,.75f,theme::Red,2);}
}
}
