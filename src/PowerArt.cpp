#include "PowerArt.h"
#include <cmath>
namespace aftertaste {
Color energyColor(EnergyMedium e){static const std::array<Color,EnergyCount> c={Color{218,170,54,255},Color{66,166,158,255},Color{121,145,151,255},Color{213,119,75,255},Color{167,159,151,255},Color{154,127,74,255},Color{123,162,78,255},Color{64,151,192,255},Color{218,96,57,255},Color{75,190,218,255}};return c.at(size_t(e));}
namespace {
struct Draw {Rectangle r;MachineAppearance a;Color ink={32,59,66,255},body={190,207,204,255},light={238,245,241,255},tone={202,163,66,255};float w;
 Draw(Rectangle bounds,MachineAppearance look):r(bounds),a(look){w=std::clamp(std::min(r.width,r.height)*.03f,1.f,2.1f);unsigned n=unsigned(std::stoul(machineSpec(a.kind).color.substr(1),nullptr,16));tone={static_cast<unsigned char>(n>>16),static_cast<unsigned char>(n>>8),static_cast<unsigned char>(n),255};if(a.monochrome){ink=BLACK;body={175,175,175,255};light=WHITE;tone={110,110,110,255};}}
 Color c(Color col)const{return a.ghost?Fade(col,.45f):col;}Vector2 p(float x,float y)const{int d=a.direction%4;float u=x,v=y;if(d==1){x=1-v;y=u;}else if(d==2){x=1-u;y=1-v;}else if(d==3){x=v;y=1-u;}return {r.x+x*r.width,r.y+y*r.height};}
 void line(float x,float y,float xx,float yy,Color col,float weight=1){DrawLineEx(p(x,y),p(xx,yy),w*weight,c(col));}
 void box(float x,float y,float width,float height,Color col){auto a=p(x,y),b=p(x+width,y+height);Rectangle q{std::min(a.x,b.x),std::min(a.y,b.y),std::abs(a.x-b.x),std::abs(a.y-b.y)};DrawRectangleRec(q,c(col));DrawRectangleLinesEx(q,w,c(ink));}
 void circle(float x,float y,float radius,Color col){auto q=p(x,y);DrawCircleV(q,radius*std::min(r.width,r.height),c(col));DrawCircleLines(int(q.x),int(q.y),radius*std::min(r.width,r.height),c(ink));}
 void feet(){box(.18f,.82f,.12f,.09f,body);box(.69f,.82f,.12f,.09f,body);}void tank(float width=.54f){box(.5f-width*.5f,.20f,width,.57f,body);circle(.5f,.20f,width*.50f,tone);box(.5f-width*.5f,.20f,width,.57f,body);line(.5f-width*.5f,.77f,.5f+width*.5f,.77f,tone,2);feet();}
 void wheel(float x,float y,float radius,int blades=6){circle(x,y,radius,body);float phase=a.active?float(a.time)*2:0;for(int n=0;n<blades;n++){float t=phase+n*6.2831853f/blades;line(x,y,x+std::cos(t)*radius*.82f,y+std::sin(t)*radius*.82f,tone,1.3f);}circle(x,y,radius*.15f,ink);}
};
}
void drawEnergyHousing(Rectangle r,const MachineAppearance& look){using K=MachineKind;Draw d(r,look);auto k=look.kind;
 if(isEnergyConduit(k)){auto col=look.monochrome?d.tone:energyColor(conduitMedium(k));d.line(.04f,.5f,.96f,.5f,d.ink,3);d.line(.04f,.5f,.96f,.5f,col,1.6f);if(k==K::DriveShaft){d.box(.18f,.36f,.15f,.28f,d.body);d.box(.66f,.36f,.15f,.28f,d.body);}else{d.circle(.25f,.5f,.065f,d.body);d.circle(.75f,.5f,.065f,d.body);}
  switch(k){
   case K::PowerCable:for(int n=0;n<6;n++)d.line(.35f+n*.06f,.50f+std::sin(n*1.4f)*.12f,.41f+n*.06f,.50f+std::sin((n+1)*1.4f)*.12f,d.ink,.6f);break;
   case K::DCCable:d.line(.35f,.36f,.65f,.36f,d.ink,.8f);d.line(.41f,.65f,.59f,.65f,d.ink,.8f);break;
   case K::SteamPipe:for(float x:{.40f,.55f,.70f}){d.line(x,.46f,x-.04f,.28f,d.ink,.7f);d.line(x-.04f,.28f,x+.02f,.17f,d.ink,.7f);}break;
   case K::ExhaustPipe:for(float x:{.40f,.55f,.70f})d.line(x,.63f,x-.04f,.77f,d.ink,.7f);break;
   case K::RawGasPipe:d.circle(.40f,.33f,.06f,d.light);d.circle(.58f,.69f,.045f,d.light);d.circle(.68f,.28f,.035f,d.light);break;
   case K::GasPipe:d.line(.45f,.30f,.61f,.50f,d.ink,.8f);d.line(.61f,.50f,.45f,.70f,d.ink,.8f);break;
   case K::ReturnPipe:d.circle(.50f,.40f,.12f,d.light);d.line(.39f,.36f,.50f,.18f,d.ink,.7f);d.line(.61f,.36f,.50f,.18f,d.ink,.7f);break;
   case K::HeatPipe:for(int n=0;n<4;n++){d.line(.35f+n*.09f,.32f,.42f+n*.09f,.71f,d.ink,.7f);}break;
   case K::CoolingPipe:d.line(.50f,.25f,.50f,.75f,d.ink,.8f);d.line(.32f,.34f,.68f,.66f,d.ink,.8f);d.line(.32f,.66f,.68f,.34f,d.ink,.8f);break;
   default:break;
  }return;}
 switch(k){
 case K::SolarPanel:d.line(.23f,.78f,.34f,.90f,d.ink,1.6f);d.line(.76f,.78f,.64f,.90f,d.ink,1.6f);d.box(.08f,.15f,.84f,.60f,d.tone);for(int n=1;n<4;n++)d.line(.08f+.21f*n,.15f,.08f+.21f*n,.75f,d.light,.5f);for(float y:{.35f,.55f})d.line(.08f,y,.92f,y,d.light,.5f);break;
 case K::WindRotor:d.box(.46f,.29f,.08f,.58f,d.body);d.box(.24f,.86f,.52f,.07f,d.ink);d.circle(.50f,.30f,.08f,d.tone);break;
 case K::Gearbox:d.feet();d.box(.13f,.30f,.74f,.48f,d.body);d.circle(.36f,.52f,.18f,d.light);d.circle(.68f,.52f,.12f,d.tone);break;
 case K::Alternator:d.feet();d.box(.13f,.32f,.71f,.43f,d.body);d.circle(.29f,.53f,.22f,d.tone);for(float x:{.51f,.62f,.73f})d.line(x,.36f,x,.71f,d.ink,.7f);d.box(.69f,.19f,.19f,.16f,d.light);break;
 case K::BiomassBoiler:d.feet();d.box(.10f,.22f,.73f,.61f,d.body);d.box(.58f,.03f,.16f,.22f,d.tone);d.box(.20f,.49f,.33f,.26f,d.ink);d.circle(.61f,.38f,.10f,d.light);break;
 case K::SteamAccumulator:d.tank(.65f);d.box(.12f,.34f,.12f,.14f,d.tone);d.box(.77f,.34f,.12f,.14f,d.tone);break;
 case K::SteamTurbine:d.feet();d.box(.12f,.31f,.73f,.45f,d.body);d.circle(.48f,.53f,.24f,d.light);d.box(.78f,.45f,.18f,.12f,d.tone);d.box(.20f,.16f,.19f,.17f,d.tone);break;
 case K::PowerCondenser:d.feet();d.box(.14f,.21f,.72f,.59f,d.body);for(float y:{.30f,.42f,.54f,.66f})d.line(.20f,y,.80f,y,d.tone,1.5f);d.line(.12f,.29f,.12f,.72f,d.ink,1.2f);break;
 case K::FeedPump:d.feet();d.circle(.40f,.55f,.25f,d.body);d.box(.60f,.39f,.27f,.31f,d.tone);d.line(.15f,.55f,.02f,.55f,d.ink,2);d.line(.48f,.30f,.48f,.13f,d.ink,2);break;
 case K::HeatExchanger:d.box(.11f,.18f,.77f,.62f,d.light);for(int n=0;n<5;n++)d.line(.20f+n*.14f,.24f,.20f+n*.14f,.75f,n%2?d.tone:d.ink,1.1f);d.box(.05f,.25f,.12f,.13f,d.tone);d.box(.83f,.62f,.12f,.13f,d.tone);break;
 case K::Macerator:d.feet();d.box(.20f,.35f,.59f,.44f,d.body);d.box(.13f,.12f,.73f,.20f,d.light);d.line(.13f,.32f,.28f,.48f,d.ink,1.1f);d.line(.86f,.32f,.72f,.48f,d.ink,1.1f);d.circle(.50f,.57f,.15f,d.tone);break;
 case K::AnaerobicDigester:d.tank(.75f);d.box(.43f,.02f,.14f,.12f,d.tone);d.box(.14f,.34f,.72f,.31f,d.light);break;
 case K::GasScrubber:d.tank(.39f);d.box(.18f,.39f,.14f,.32f,d.tone);for(float y:{.30f,.42f,.54f,.66f})d.line(.36f,y,.65f,y,d.ink,.6f);d.line(.78f,.19f,.78f,.78f,d.ink,1.3f);break;
 case K::GasHolder:d.box(.13f,.55f,.74f,.27f,d.body);d.box(.18f,.18f,.64f,.45f,d.light);d.line(.11f,.18f,.11f,.84f,d.ink,1.4f);d.line(.89f,.18f,.89f,.84f,d.ink,1.4f);d.box(.16f,.15f,.68f,.08f,d.tone);break;
 case K::GasEngine:d.feet();d.box(.09f,.43f,.68f,.36f,d.body);for(float x:{.19f,.38f,.57f})d.box(x,.25f,.14f,.20f,d.tone);d.circle(.78f,.59f,.17f,d.light);d.line(.72f,.27f,.90f,.27f,d.ink,1.3f);d.line(.90f,.27f,.90f,.08f,d.ink,1.3f);break;
 case K::BatteryRack:d.feet();d.box(.13f,.15f,.74f,.66f,d.ink);for(int y=0;y<3;y++)for(int x=0;x<2;x++)d.box(.20f+x*.32f,.23f+y*.17f,.25f,.12f,d.body);d.line(.31f,.06f,.31f,.15f,d.tone,1.8f);d.line(.68f,.06f,.68f,.15f,d.tone,1.8f);break;
 case K::Flywheel:d.feet();d.circle(.50f,.48f,.35f,d.ink);d.circle(.50f,.48f,.27f,d.body);break;
 case K::Inverter:d.feet();d.box(.17f,.16f,.66f,.65f,d.body);d.box(.28f,.25f,.45f,.22f,d.light);for(float y:{.58f,.65f,.72f})d.line(.26f,y,.74f,y,d.ink,.7f);break;
 case K::Switchboard:d.feet();d.box(.12f,.12f,.77f,.71f,d.body);for(float x:{.25f,.49f,.73f}){d.box(x-.06f,.27f,.12f,.14f,d.light);d.box(x-.05f,.52f,.10f,.20f,d.ink);}break;
 case K::PowerRelay:d.box(.19f,.25f,.63f,.53f,d.light);d.circle(.50f,.49f,.15f,d.tone);d.line(.12f,.37f,.19f,.37f,d.ink,1.4f);d.line(.82f,.63f,.94f,.63f,d.ink,1.4f);break;
 case K::GridInlet:d.box(.14f,.21f,.70f,.60f,d.body);d.circle(.49f,.49f,.22f,d.light);for(float x:{.24f,.50f,.76f})d.line(x,.02f,x,.20f,d.ink,1.6f);break;
 case K::HeatTank:d.tank(.72f);d.box(.34f,.27f,.32f,.39f,d.light);d.line(.50f,.32f,.50f,.61f,d.tone,2);break;
 case K::CoolingPlant:d.feet();d.box(.08f,.24f,.85f,.58f,d.body);d.circle(.34f,.50f,.22f,d.light);d.circle(.72f,.50f,.13f,d.tone);d.box(.13f,.12f,.73f,.10f,d.tone);break;
 default:break;}
 Machine portMachine;portMachine.kind=k;portMachine.dir=0;
 auto ports=energyPorts(portMachine);for(size_t n=0;n<ports.size();n++){auto port=ports[n];float shift=0;for(size_t j=0;j<ports.size();j++)if(j!=n&&ports[j].direction==port.direction)shift=(j<n?.07f:-.07f);float px=port.direction==0?.96f:port.direction==2?.04f:.5f+shift,py=port.direction==1?.96f:port.direction==3?.04f:.5f+shift;
  Color c=look.monochrome?d.tone:energyColor(port.medium);d.box(px-.035f,py-.035f,.07f,.07f,c);
  float dx=port.direction==0?1:port.direction==2?-1:0,dy=port.direction==1?1:port.direction==3?-1:0;float sign=port.output?1.f:-1.f;float ex=px+dx*.025f*sign,ey=py+dy*.025f*sign;
  d.line(ex-dx*.025f*sign-dy*.02f,ey-dy*.025f*sign+dx*.02f,ex,ey,d.ink,.5f);d.line(ex-dx*.025f*sign+dy*.02f,ey-dy*.025f*sign-dx*.02f,ex,ey,d.ink,.5f);
 }
 if(acceptsEnergyFuel(k)){d.box(.01f,.45f,.07f,.10f,d.body);if(k==K::Macerator)d.box(.92f,.45f,.07f,.10f,d.body);if(hasSideOutlet(k))d.box(.45f,.92f,.10f,.07f,look.monochrome?d.tone:Color{226,113,61,255});}
}
void drawEnergyMechanism(Rectangle r,const MachineAppearance& look){using K=MachineKind;Draw d(r,look);float t=look.active?float(look.time):0;double fill=std::clamp(look.progress,0.,1.);
 switch(look.kind){
 case K::WindRotor:for(int n=0;n<3;n++){float angle=t*2+n*2*PI/3;d.line(.50f,.30f,.50f+std::cos(angle)*.33f,.30f+std::sin(angle)*.27f,d.ink,3);d.line(.50f,.30f,.50f+std::cos(angle)*.33f,.30f+std::sin(angle)*.27f,d.light,1.7f);}d.circle(.5f,.3f,.07f,d.tone);break;
 case K::Gearbox:d.wheel(.36f,.52f,.12f,5);d.wheel(.68f,.52f,.07f,4);break;
 case K::Alternator:d.wheel(.29f,.53f,.14f,4);break;
 case K::SteamTurbine:d.wheel(.48f,.53f,.16f,8);break;
 case K::FeedPump:d.wheel(.40f,.55f,.16f,3);break;
 case K::Flywheel:d.wheel(.50f,.48f,.22f,5);break;
 case K::CoolingPlant:d.wheel(.34f,.50f,.15f,4);d.wheel(.72f,.50f,.08f,3);break;
 case K::BiomassBoiler:if(look.active){for(int n=0;n<3;n++){float y=.70f-.06f*std::sin(t*4+n);d.line(.27f+n*.08f,.71f,.29f+n*.08f,y-.08f,d.tone,2);}}d.line(.61f,.38f,.65f,.31f,d.ink,.7f);break;
 case K::BatteryRack:for(int n=0;n<6;n++)if(n<fill*6){int x=n%2,y=n/2;d.box(.22f+x*.32f,.25f+y*.17f,.20f,.08f,d.tone);}break;
 case K::GasHolder:d.box(.17f,.64f-float(fill)*.42f,.67f,.08f,d.tone);break;
 case K::SteamAccumulator:case K::HeatTank:d.line(.50f,.69f,.50f,.69f-float(fill)*.35f,d.tone,3);break;
 case K::AnaerobicDigester:for(int n=0;n<4;n++){float rise=look.active?std::fmod(t*.17f+n*.24f,1.f):n*.20f;d.circle(.27f+n*.15f,.62f-rise*.24f,.022f,d.tone);}break;
 case K::GasEngine:for(int n=0;n<3;n++){float y=.41f+(look.active?std::sin(t*4+n*2)*.035f:0);d.line(.25f+n*.19f,y,.25f+n*.19f,.65f,d.ink,.9f);}d.wheel(.78f,.59f,.10f,4);break;
 case K::Switchboard:for(float x:{.25f,.49f,.73f})d.line(x,.56f,x+(look.closed?.06f:0),look.closed?.68f:.62f,look.closed?d.tone:d.light,2);break;
 case K::Inverter:for(int n=0;n<9;n++){float x=.31f+n*.043f,y=.36f+.06f*std::sin(n*.9f+t*2);if(n)d.line(x-.043f,.36f+.06f*std::sin((n-1)*.9f+t*2),x,y,d.tone,.7f);}break;
 case K::Macerator:d.wheel(.50f,.57f,.09f,3);break;
 case K::PowerRelay:case K::GridInlet:d.line(.50f,.49f,.50f+std::cos(float(fill)*2-2.5f)*.13f,.49f+std::sin(float(fill)*2-2.5f)*.13f,d.ink,.9f);break;
 default:break;}
}
}
