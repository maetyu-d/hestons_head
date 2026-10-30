#include "PowerArt.h"
#include "ReagentWorks.h"
#include "RouteArt.h"
#include "WaterWorks.h"
#include <cmath>
namespace aftertaste {
Color energyColor(EnergyMedium e){static const std::array<Color,EnergyCount> c={Color{218,170,54,255},Color{66,166,158,255},Color{121,145,151,255},Color{213,119,75,255},Color{167,159,151,255},Color{154,127,74,255},Color{123,162,78,255},Color{64,151,192,255},Color{218,96,57,255},Color{75,190,218,255},Color{155,140,181,255},Color{177,133,163,255},Color{117,155,155,255},Color{98,182,178,255},Color{105,191,214,255},Color{37,159,201,255},Color{148,139,114,255},Color{188,146,87,255}};return c.at(size_t(e));}
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
void drawEnergyHousing(Rectangle r,const MachineAppearance& look){using K=MachineKind;Draw d(r,look);auto k=look.kind;if(k==K::RainCollector||k==K::WaterAnalyzer)d.a.direction=0;
 if(isEnergyConduit(k)){auto col=look.monochrome?d.tone:energyColor(conduitMedium(k));auto family=lineFamily(conduitMedium(k));
  if(family==LineFamily::Water){d.line(.04f,.5f,.96f,.5f,d.ink,4);d.line(.04f,.5f,.96f,.5f,d.light,2.8f);d.line(.04f,.5f,.96f,.5f,col,1);for(float x:{.25f,.75f}){d.circle(x,.5f,.065f,d.light);d.circle(x,.5f,.027f,col);}}
  else if(family==LineFamily::Electricity){d.line(.04f,.5f,.96f,.5f,d.ink,1.7f);d.line(.04f,.5f,.96f,.5f,col,.65f);for(float x:{.25f,.75f}){d.box(x-.025f,.445f,.05f,.11f,d.ink);d.line(x,.46f,x,.54f,col,.7f);}}
  else{d.line(.04f,.5f,.96f,.5f,d.ink,3);d.line(.04f,.5f,.96f,.5f,col,1.6f);}
  if(k==K::DriveShaft){d.box(.18f,.36f,.15f,.28f,d.body);d.box(.66f,.36f,.15f,.28f,d.body);}else if(family==LineFamily::Service){d.circle(.25f,.5f,.065f,d.body);d.circle(.75f,.5f,.065f,d.body);}
  switch(k){
   case K::VacuumHose:d.circle(.50f,.50f,.17f,d.light);d.line(.39f,.50f,.61f,.50f,d.tone,1.1f);break;
   case K::InstrumentLead:for(int n=0;n<5;n++)d.line(.30f+n*.09f,.39f+(n%2)*.22f,.39f+n*.09f,.61f-(n%2)*.22f,d.tone,.7f);break;
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
 if(isReagentMachine(k)){
  // Lab glass remains upright; only the physical material ports rotate.
  d.a.direction=0;d.feet();
  switch(k){
   case K::MineralHopper:d.box(.16f,.15f,.68f,.16f,d.tone);d.line(.18f,.32f,.38f,.66f,d.ink,1.3f);d.line(.82f,.32f,.62f,.66f,d.ink,1.3f);d.box(.38f,.65f,.24f,.15f,d.body);break;
   case K::ReagentExtractor:d.box(.13f,.73f,.74f,.09f,d.tone);d.circle(.5f,.48f,.28f,d.light);d.box(.43f,.12f,.14f,.20f,d.light);d.line(.50f,.13f,.50f,.67f,d.ink,.8f);d.box(.35f,.09f,.30f,.07f,d.body);break;
   case K::CultureVat:d.tank(.61f);d.box(.26f,.28f,.48f,.41f,d.light);d.line(.50f,.08f,.50f,.67f,d.ink,.9f);d.box(.36f,.05f,.28f,.10f,d.tone);d.circle(.84f,.25f,.09f,d.light);d.line(.76f,.25f,.70f,.25f,d.ink,1);break;
   case K::SaltReactor:d.box(.10f,.71f,.80f,.11f,d.body);for(float x:{.31f,.70f}){d.circle(x,.49f,.19f,d.light);d.box(x-.045f,.16f,.09f,.17f,d.light);}d.line(.31f,.17f,.70f,.17f,d.tone,1);d.box(.42f,.67f,.16f,.10f,d.tone);break;
   case K::ReagentSeparator:d.box(.12f,.20f,.76f,.58f,d.body);d.circle(.49f,.48f,.25f,d.light);d.circle(.49f,.48f,.08f,d.tone);for(int n=0;n<3;n++){float t=n*2.094f;d.line(.49f,.48f,.49f+std::cos(t)*.19f,.48f+std::sin(t)*.19f,d.tone,1.4f);}break;
   case K::ReagentEvaporator:d.box(.13f,.72f,.72f,.11f,d.tone);d.circle(.35f,.53f,.23f,d.light);d.box(.31f,.18f,.08f,.16f,d.light);d.line(.36f,.20f,.65f,.20f,d.ink,1);d.box(.62f,.26f,.17f,.34f,d.body);for(float y:{.31f,.40f,.49f})d.line(.65f,y,.76f,y,d.tone,1);d.line(.70f,.60f,.85f,.73f,d.tone,1);break;
   case K::ReagentPolisher:d.box(.33f,.10f,.34f,.67f,d.light);for(float y:{.24f,.37f,.50f,.63f}){d.box(.36f,y,.28f,.045f,d.tone);}d.box(.22f,.76f,.56f,.07f,d.body);break;
   case K::ReagentTank:d.box(.41f,.10f,.18f,.20f,d.light);d.circle(.50f,.55f,.28f,d.light);d.box(.23f,.71f,.54f,.09f,d.body);for(float y:{.43f,.53f,.63f})d.line(.67f,y,.74f,y,d.ink,.55f);break;
   case K::ReagentAnalyzer:d.box(.12f,.25f,.76f,.52f,d.body);d.box(.23f,.34f,.42f,.25f,d.ink);d.circle(.75f,.40f,.05f,d.tone);d.circle(.75f,.59f,.04f,d.light);break;
   case K::ReagentIntake:d.box(.17f,.22f,.66f,.56f,d.light);d.box(.29f,.32f,.42f,.25f,d.tone);d.line(.35f,.46f,.45f,.55f,d.light,1.3f);d.line(.45f,.55f,.65f,.36f,d.light,1.3f);d.box(.30f,.68f,.40f,.07f,d.body);break;
   case K::ReagentRecovery:d.box(.23f,.22f,.54f,.54f,d.light);d.box(.32f,.14f,.36f,.10f,d.tone);d.line(.32f,.64f,.68f,.64f,d.ink,1);for(float x:{.35f,.50f,.65f})d.circle(x,.69f,.025f,d.tone);break;
   default:break;
  }
  Draw ports(r,look);
  if(k==K::ReagentAnalyzer)ports.line(.88f,.5f,.99f,.5f,d.ink,1.2f);
  Color glassCollar=look.monochrome?d.ink:Color{146,86,155,255};
  auto glassPort=[&](float x,float y,bool vertical,bool side){ports.box(x,y,vertical?.12f:.08f,vertical?.08f:.12f,d.light);Color collar=side&&!look.monochrome?Color{201,111,65,255}:glassCollar;if(vertical){ports.line(x,y+.015f,x+.12f,y+.015f,collar,.65f);ports.line(x,y+.065f,x+.12f,y+.065f,collar,.65f);}else{ports.line(x+.015f,y,x+.015f,y+.12f,collar,.65f);ports.line(x+.065f,y,x+.065f,y+.12f,collar,.65f);}};
  if(k!=K::MineralHopper&&k!=K::ReagentAnalyzer)glassPort(.01f,.44f,false,false);
  if(k!=K::ReagentRecovery&&k!=K::ReagentIntake&&k!=K::ReagentAnalyzer)glassPort(.91f,.44f,false,false);
  if(hasSideOutlet(k))glassPort(.44f,.91f,true,true);
  // Separate gold AC tap, away from food and side-fraction couplings.
  d.box(.10f,.84f,.07f,.07f,energyColor(EnergyMedium::AC));
  return;
 }
 switch(k){
 case K::BoreholePump:d.feet();d.box(.10f,.69f,.80f,.13f,d.body);d.box(.40f,.35f,.20f,.35f,d.light);d.circle(.50f,.35f,.21f,d.body);d.line(.50f,.83f,.50f,.99f,d.tone,2);d.box(.25f,.12f,.50f,.10f,d.tone);break;
 case K::RainCollector:d.box(.10f,.49f,.80f,.12f,d.body);d.line(.10f,.35f,.50f,.10f,d.ink,1.4f);d.line(.50f,.10f,.90f,.35f,d.ink,1.4f);d.box(.25f,.62f,.50f,.23f,d.light);d.line(.50f,.61f,.50f,.75f,d.tone,2);break;
 case K::RawWaterTank:case K::CleanWaterTank:case K::ConcentrateTank:d.tank(.65f);d.box(.23f,.27f,.54f,.43f,d.light);d.line(.32f,.13f,.68f,.13f,d.tone,1.6f);for(float y:{.35f,.45f,.55f,.65f})d.line(.78f,y,.86f,y,d.ink,.6f);break;
 case K::SandFilter:d.feet();d.box(.25f,.10f,.50f,.68f,d.light);d.box(.29f,.19f,.42f,.15f,d.body);d.box(.29f,.36f,.42f,.14f,d.tone);for(int n=0;n<5;n++)d.circle(.34f+n*.08f,.61f,.035f,d.body);d.line(.75f,.15f,.88f,.15f,d.ink,1.2f);d.line(.88f,.15f,.88f,.69f,d.ink,1.2f);break;
 case K::MembraneFilter:d.feet();d.box(.10f,.22f,.80f,.48f,d.body);for(int n=0;n<3;n++){d.box(.19f,.29f+n*.11f,.55f,.085f,d.light);d.circle(.77f,.33f+n*.11f,.04f,d.tone);}d.circle(.24f,.76f,.12f,d.light);d.circle(.76f,.13f,.08f,d.light);break;
 case K::UVSterilizer:d.feet();d.box(.10f,.31f,.80f,.39f,d.body);d.box(.16f,.38f,.68f,.24f,d.light);d.line(.22f,.50f,.78f,.50f,d.tone,3);d.box(.31f,.15f,.38f,.12f,d.tone);break;
 case K::WaterOutlet:d.feet();d.box(.10f,.19f,.56f,.59f,d.light);d.circle(.37f,.40f,.14f,d.body);d.line(.20f,.65f,.65f,.65f,d.tone,2);d.line(.65f,.65f,.84f,.48f,d.tone,2);d.line(.66f,.48f,.84f,.48f,d.tone,1.5f);d.line(.84f,.48f,.84f,.65f,d.tone,1.5f);break;
 case K::GreywaterDrain:d.box(.10f,.54f,.80f,.28f,d.body);for(float x:{.22f,.36f,.50f,.64f,.78f})d.line(x,.56f,x,.74f,d.ink,1);d.line(.23f,.17f,.23f,.45f,d.tone,1.8f);d.line(.13f,.34f,.23f,.45f,d.tone,1.2f);d.line(.33f,.34f,.23f,.45f,d.tone,1.2f);break;
 case K::ReedReclaimer:d.box(.10f,.59f,.80f,.25f,d.body);for(float x:{.25f,.43f,.62f,.78f}){d.line(x,.59f,x,.22f,d.tone,1.1f);d.line(x,.45f,x-.10f,.31f,d.tone,1.1f);d.line(x,.38f,x+.09f,.24f,d.tone,1.1f);}d.box(.13f,.65f,.74f,.10f,d.light);break;
 case K::WaterAnalyzer:{d.feet();d.box(.18f,.19f,.64f,.56f,d.body);d.box(.27f,.29f,.46f,.25f,d.ink);d.circle(.32f,.65f,.045f,d.tone);d.circle(.51f,.65f,.045f,d.light);int direction=look.direction%4;float sx=direction==0?.82f:direction==2?.18f:.5f,sy=direction==1?.75f:direction==3?.19f:.47f;d.line(sx,sy,direction==0?.99f:direction==2?.01f:.5f,direction==1?.99f:direction==3?.01f:.47f,d.ink,1.2f);break;}
 case K::CuriosityPile:
  d.feet();d.box(.10f,.73f,.80f,.10f,d.body);d.box(.17f,.27f,.66f,.46f,d.body);
  d.circle(.50f,.28f,.28f,d.light);d.box(.23f,.28f,.54f,.43f,d.light);
  for(float x:{.18f,.74f})d.box(x,.33f,.08f,.36f,d.body);
  d.box(.32f,.43f,.36f,.24f,d.ink);d.box(.44f,.45f,.12f,.19f,d.tone);
  d.line(.50f,.10f,.50f,.42f,d.ink,1.2f);d.box(.36f,.09f,.28f,.07f,d.tone);
  d.circle(.76f,.20f,.09f,d.light);d.line(.76f,.20f,.80f,.15f,d.ink,.7f);break;
 case K::StarChamber:
  d.feet();d.box(.13f,.76f,.74f,.08f,d.body);d.circle(.50f,.38f,.31f,d.light);d.box(.20f,.38f,.60f,.36f,d.light);
  d.box(.17f,.69f,.66f,.08f,d.body);d.box(.13f,.31f,.09f,.29f,d.tone);d.box(.78f,.31f,.09f,.29f,d.tone);
  d.line(.27f,.38f,.73f,.38f,d.ink,.6f);d.line(.34f,.27f,.34f,.63f,d.ink,.6f);d.line(.66f,.27f,.66f,.63f,d.ink,.6f);
  d.circle(.50f,.44f,.15f,d.body);d.circle(.50f,.44f,.09f,d.light);d.box(.41f,.10f,.18f,.09f,d.body);break;
 case K::LabVacuumPump:
  d.feet();d.box(.12f,.65f,.77f,.16f,d.body);d.circle(.29f,.58f,.18f,d.tone);d.box(.19f,.47f,.29f,.20f,d.body);
  for(float x:{.58f,.79f}){d.circle(x,.49f,.11f,d.light);d.box(x-.035f,.22f,.07f,.18f,d.light);d.line(x,.51f,x,.60f,d.tone,1.5f);}
  d.line(.40f,.40f,.40f,.24f,d.ink,.8f);d.line(.40f,.24f,.80f,.24f,d.ink,.8f);d.circle(.20f,.26f,.08f,d.light);break;
 case K::QuenchBath:
  d.feet();d.box(.11f,.29f,.78f,.48f,d.light);d.box(.14f,.45f,.72f,.28f,d.tone);
  for(int n=0;n<5;n++)d.line(.26f+n*.11f,.35f,.26f+n*.11f,.66f,d.ink,.9f);
  d.box(.09f,.23f,.82f,.09f,d.body);d.box(.65f,.11f,.17f,.13f,d.body);d.line(.74f,.12f,.74f,.59f,d.ink,.8f);break;
 case K::Calorimeter:
  d.feet();d.box(.10f,.75f,.80f,.08f,d.body);
  d.circle(.31f,.50f,.20f,d.light);d.circle(.71f,.50f,.20f,d.light);
  d.box(.27f,.19f,.08f,.15f,d.light);d.box(.67f,.19f,.08f,.15f,d.light);
  d.line(.31f,.27f,.71f,.27f,d.ink,.7f);d.line(.49f,.27f,.49f,.11f,d.ink,.7f);
  d.box(.43f,.08f,.14f,.11f,d.tone);d.line(.31f,.70f,.31f,.75f,d.ink,.8f);d.line(.71f,.70f,.71f,.75f,d.ink,.8f);
  d.line(.51f,.47f,.51f,.61f,d.tone,2);break;
 case K::ExperimentConsole:
  d.feet();d.box(.09f,.20f,.82f,.57f,d.body);d.box(.17f,.28f,.48f,.30f,d.ink);d.box(.71f,.28f,.13f,.30f,d.light);
  d.circle(.26f,.67f,.045f,d.tone);d.circle(.42f,.67f,.045f,d.tone);d.box(.58f,.64f,.26f,.05f,d.ink);
  for(float y:{.33f,.40f,.47f})d.line(.73f,y,.82f,y,d.tone,.6f);
  d.line(.71f,.77f,.71f,.88f,d.ink,.7f);d.line(.71f,.88f,.88f,.88f,d.ink,.7f);break;
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
 Machine portMachine;portMachine.kind=k;portMachine.dir=(k==K::RainCollector||k==K::WaterAnalyzer)?look.direction:0;
 auto ports=energyPorts(portMachine);for(size_t n=0;n<ports.size();n++){auto port=ports[n];float shift=0;for(size_t j=0;j<ports.size();j++)if(j!=n&&ports[j].direction==port.direction)shift=(j<n?.07f:-.07f);float px=port.direction==0?.96f:port.direction==2?.04f:.5f+shift,py=port.direction==1?.96f:port.direction==3?.04f:.5f+shift;
  if(lineFamily(port.medium)==LineFamily::Electricity&&(isWaterMachine(k))){if(port.direction%2==0)py=.82f;else px=.82f;}
  Color c=look.monochrome?d.tone:energyColor(port.medium);if(lineFamily(port.medium)==LineFamily::Water){d.circle(px,py,.046f,d.light);d.circle(px,py,.021f,c);}else d.box(px-.035f,py-.035f,.07f,.07f,c);
  float dx=port.direction==0?1:port.direction==2?-1:0,dy=port.direction==1?1:port.direction==3?-1:0;float sign=port.output?1.f:-1.f;float ex=px+dx*.025f*sign,ey=py+dy*.025f*sign;
  d.line(ex-dx*.025f*sign-dy*.02f,ey-dy*.025f*sign+dx*.02f,ex,ey,d.ink,.5f);d.line(ex-dx*.025f*sign+dy*.02f,ey-dy*.025f*sign-dx*.02f,ex,ey,d.ink,.5f);
 }
 if(acceptsEnergyFuel(k)){d.box(.01f,.45f,.07f,.10f,d.body);if(k==K::Macerator)d.box(.92f,.45f,.07f,.10f,d.body);if(hasSideOutlet(k))d.box(.45f,.92f,.10f,.07f,look.monochrome?d.tone:Color{226,113,61,255});}
}
void drawEnergyMechanism(Rectangle r,const MachineAppearance& look){
 if(isReagentMachine(look.kind)){
  Draw d(r,look);d.a.direction=0;
  if(look.reagentFill>0&&look.kind!=MachineKind::ReagentAnalyzer){
   Color color=d.tone;if(!look.reagentKey.empty()){auto value=ingredient(look.reagentKey).color;unsigned n=unsigned(std::stoul(value.substr(1),nullptr,16));color={static_cast<unsigned char>(n>>16),static_cast<unsigned char>(n>>8),static_cast<unsigned char>(n),255};}
   float level=.12f+.20f*float(std::clamp(look.reagentFill,0.,1.));
   // A bounded liquid viewport exposes the live lot rather than a decorative fill.
   d.box(.36f,.64f-level,.28f,level,look.monochrome?d.body:Fade(color,.8f));
   if(look.active&&look.kind==MachineKind::CultureVat)for(int n=0;n<3;n++){float rise=float(std::fmod(look.time*.12+n*.31,1.));d.circle(.42f+n*.075f,.61f-rise*.22f,.018f,d.light);}
   if(look.reagentPurity<.85)for(int n=0;n<3;n++)d.circle(.40f+n*.075f,.60f,.012f,d.ink);
  }
  if(look.kind==MachineKind::ReagentAnalyzer){for(int n=0;n<5;n++)d.line(.27f+n*.055f,.52f,.27f+n*.055f,.43f-.04f*std::sin(n+look.time*.1f),d.tone,.7f);}
  return;
 }
using K=MachineKind;Draw d(r,look);if(look.kind==K::RainCollector||look.kind==K::WaterAnalyzer)d.a.direction=0;float t=look.active?float(look.time):0;double fill=std::clamp(look.progress,0.,1.);
 switch(look.kind){
 case K::BoreholePump:d.wheel(.50f,.35f,.13f,4);break;
 case K::RainCollector:if(look.active)for(int n=0;n<4;n++){float y=.12f+float(std::fmod(t*.22+n*.22,1.))*.33f;d.line(.20f+n*.19f,y,.17f+n*.19f,y+.07f,d.tone,.7f);}break;
 case K::RawWaterTank:case K::CleanWaterTank:case K::ConcentrateTank:{Color water=d.tone;if(!look.monochrome&&look.waterDirt>1)water={145,139,106,255};float h=float(fill)*.37f;d.box(.27f,.67f-h,.46f,std::max(.004f,h),water);if(look.waterBio>.05)for(int n=0;n<3;n++)d.circle(.34f+n*.14f,.58f-.04f*std::sin(t+n),.018f,d.light);break;}
 case K::SandFilter:for(int n=0;n<3;n++){float y=.18f+float(std::fmod(t*.15+n*.3,1.))*.48f;d.circle(.50f,y,.014f,d.tone);}break;
 case K::MembraneFilter:d.wheel(.24f,.76f,.075f,3);for(int n=0;n<3;n++){float x=.23f+float(std::fmod(t*.2+n*.3,1.))*.40f;d.line(x,.33f+n*.11f,x+.055f,.33f+n*.11f,d.tone,.7f);}break;
 case K::UVSterilizer:for(int n=0;n<4;n++){float x=.24f+n*.16f;d.line(x,.36f,x+.03f*std::sin(t*2+n),.63f,d.tone,.8f);}break;
 case K::WaterOutlet:d.line(.37f,.40f,.44f,.33f+float(fill)*.10f,d.ink,.7f);break;
 case K::GreywaterDrain:for(int n=0;n<3;n++){float x=.46f+n*.15f;d.circle(x,.45f+float(std::fmod(t*.2+n*.23,1.))*.10f,.014f,d.tone);}break;
 case K::ReedReclaimer:for(float x:{.25f,.43f,.62f,.78f})d.line(x,.56f,x+.025f*std::sin(t+x*9),.20f,d.tone,.8f);break;
 case K::WaterAnalyzer:{std::array<double,3> readings={look.waterDirt/2,look.waterSalt/6,look.waterBio/4};for(int n=0;n<3;n++)d.line(.34f+n*.13f,.48f,.34f+n*.13f,.48f-float(std::clamp(readings[size_t(n)],0.,1.))*.16f,d.tone,1.4f);break;}
 case K::CuriosityPile:{
  float glow=float(look.labFlux);d.circle(.50f,.54f,.055f+glow*.03f,d.tone);
  for(int n=0;n<3;n++){float y=.66f-float(std::fmod(t*.12+n*.22,1.))*.23f;d.circle(.31f+n*.18f,y,.012f+glow*.009f,d.tone);}
  d.line(.50f,.16f,.50f,.39f-float(look.labFlux)*.17f,d.body,2);d.circle(.76f,.20f,.035f,look.labArmed?d.tone:d.body);break;}
 case K::StarChamber:{
  float glow=float(look.labFlux);d.circle(.50f,.44f,.04f+glow*.08f,d.tone);
  for(int n=0;n<6;n++){float a=n*PI/3+t*.2f;d.line(.50f,.44f,.50f+std::cos(a)*(.07f+glow*.16f),.44f+std::sin(a)*(.07f+glow*.16f),d.tone,.6f);}
  for(int n=0;n<3;n++)d.line(.24f,.64f+n*.035f,.24f+float(look.labVacuum)*.50f,.64f+n*.035f,d.tone,.5f);break;}
 case K::LabVacuumPump:d.wheel(.29f,.58f,.09f,4);d.line(.20f,.26f,.20f+.04f*float(look.progress),.20f,d.ink,.7f);break;
 case K::QuenchBath:for(int n=0;n<4;n++){float y=.68f-float(std::fmod(t*.15+n*.23,1.))*.20f;d.circle(.20f+n*.16f,y,.013f,d.light);}break;
 case K::Calorimeter:d.line(.31f,.57f,.31f,.39f,d.tone,2);d.line(.71f,.57f,.71f,.50f-float(look.labCooling)*.10f,d.tone,2);d.circle(.31f,.59f,.055f,d.tone);d.circle(.71f,.59f,.055f,d.body);break;
 case K::ExperimentConsole:for(int n=1;n<16;n++){float a=(n-1)/15.f,b=n/15.f;auto wave=[&](float x){return .43f+std::sin(x*13+t)*(.02f+float(look.labStability)*.07f);};d.line(.20f+a*.42f,wave(a),.20f+b*.42f,wave(b),d.tone,.6f);}break;
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
