#include "Theme.h"
#include "RouteArt.h"
#include <algorithm>
#include <cmath>
namespace aftertaste {
namespace {
const Color floorColor{255,255,255,255},cableJacket{126,87,13,255},pipeEdge{31,83,139,255},serviceEdge{55,73,77,255};
Color blend(Color a,Color b,float t){return {static_cast<unsigned char>(a.r+(b.r-a.r)*t),static_cast<unsigned char>(a.g+(b.g-a.g)*t),static_cast<unsigned char>(a.b+(b.b-a.b)*t),255};}
float length(Vector2 a,Vector2 b){return std::abs(b.x-a.x)+std::abs(b.y-a.y);}
void stroke(const std::vector<Vector2>& p,float width,Color c){
 for(size_t n=1;n<p.size();n++){auto a=p[n-1],b=p[n];if(length(a,b)<.001f)continue;DrawLineEx(a,b,width,c);if(n+1<p.size())DrawRectangleRec({b.x-width*.5f,b.y-width*.5f,width,width},c);}
}
Vector2 point(const std::vector<Vector2>& p,float t){float total=0;for(size_t n=1;n<p.size();n++)total+=length(p[n-1],p[n]);float distance=std::clamp(t,0.f,1.f)*total;for(size_t n=1;n<p.size();n++){float span=length(p[n-1],p[n]);if(distance<=span&&span>.001f){float u=distance/span;return {p[n-1].x+(p[n].x-p[n-1].x)*u,p[n-1].y+(p[n].y-p[n-1].y)*u};}distance-=span;}return p.empty()?Vector2{}:p.back();}
}
LineFamily lineFamily(EnergyMedium e){if(e==EnergyMedium::AC||e==EnergyMedium::DC)return LineFamily::Electricity;if(e==EnergyMedium::Water||e==EnergyMedium::Cooling||(e>=EnergyMedium::RawWater&&e<=EnergyMedium::Concentrate))return LineFamily::Water;return LineFamily::Service;}
float infrastructureWidth(EnergyMedium e,float tile){auto family=lineFamily(e);return family==LineFamily::Electricity?std::clamp(tile*.034f,1.4f,2.7f):family==LineFamily::Water?std::clamp(tile*.092f,3.2f,6.8f):std::clamp(tile*.045f,1.6f,3.4f);}
void drawInfrastructureRoute(const std::vector<Vector2>& p,EnergyMedium e,float tile,bool enabled,bool tripped,bool focused){
 if(p.size()<2)return;float w=infrastructureWidth(e,tile);Color color=energyColor(e);if(!enabled)color=blend(color,floorColor,.4f);if(tripped)color={204,89,61,255};
 auto family=lineFamily(e);
 if(family==LineFamily::Electricity){
  // An insulated square tray passes visibly over pipework at crossings.
  stroke(p,w+2.4f,floorColor);stroke(p,w+.8f,cableJacket);stroke(p,std::max(.95f,w*.82f),color);
 }else if(family==LineFamily::Water){
  stroke(p,w,enabled?pipeEdge:blend(pipeEdge,floorColor,.55f));stroke(p,std::max(1.4f,w-1.4f),blend(color,WHITE,.08f));stroke(p,std::max(.8f,w*.5f),color);
  stroke(p,std::max(.55f,w*.10f),blend(color,WHITE,.85f));
 }else{
  stroke(p,w,blend(floorColor,serviceEdge,focused?.63f:.45f));stroke(p,std::max(.8f,w-1.f),blend(color,WHITE,.40f));
 }
}
void drawInfrastructureFitting(Vector2 at,Vector2 tangent,EnergyMedium e,float tile,bool enabled){
 if(tile<24)return;float w=infrastructureWidth(e,tile);bool horizontal=std::abs(tangent.x)>=std::abs(tangent.y);Color color=enabled?energyColor(e):Color{151,163,160,255};
 if(lineFamily(e)==LineFamily::Water){float radius=std::max(2.1f,w*.78f);DrawCircleV(at,radius,pipeEdge);DrawCircleV(at,std::max(1.f,radius-1.1f),blend(color,WHITE,.20f));DrawCircleV(at,std::max(.8f,radius*.36f),color);}
 else if(lineFamily(e)==LineFamily::Electricity){float across=std::max(4.f,w+2),along=std::max(2.f,w*.9f);Rectangle clip{at.x-(horizontal?along:across)*.5f,at.y-(horizontal?across:along)*.5f,horizontal?along:across,horizontal?across:along};DrawRectangleRec(clip,cableJacket);DrawRectangleRec({clip.x+.6f,clip.y+.6f,std::max(.7f,clip.width-1.2f),std::max(.7f,clip.height-1.2f)},color);if(horizontal)DrawLineEx({at.x,at.y-across*.25f},{at.x,at.y+across*.25f},.9f,color);else DrawLineEx({at.x-across*.25f,at.y},{at.x+across*.25f,at.y},.9f,color);}
 else{float s=std::max(2.f,w*.75f);DrawRectangleRec({at.x-s*.5f,at.y-s*.5f,s,s},color);}
}
void drawInfrastructureJunction(Vector2 at,EnergyMedium e,float tile){float w=infrastructureWidth(e,tile);if(lineFamily(e)==LineFamily::Water){DrawCircleV(at,w*.72f,pipeEdge);DrawCircleV(at,std::max(1.f,w*.47f),blend(energyColor(e),WHITE,.15f));}else{float s=w+1.8f;DrawRectangleRec({at.x-s*.5f,at.y-s*.5f,s,s},lineFamily(e)==LineFamily::Electricity?cableJacket:serviceEdge);DrawRectangleRec({at.x-w*.3f,at.y-w*.3f,w*.6f,w*.6f},energyColor(e));}}
void drawInfrastructureFlow(const std::vector<Vector2>& path,EnergyMedium e,float tile,float phase,bool reverse){
 if(path.size()<2)return;if(reverse)phase=1-phase;auto p=point(path,phase);float w=infrastructureWidth(e,tile);
 if(lineFamily(e)==LineFamily::Electricity){float size=std::max(1.3f,w*.7f);DrawRectangleRec({p.x-size*.5f,p.y-size*.5f,size,size},Color{255,241,160,255});}
 else if(lineFamily(e)==LineFamily::Water){float radius=std::max(.9f,w*.26f);DrawCircleV(p,radius,energyColor(e));DrawCircleV({p.x-radius*.25f,p.y-radius*.25f},std::max(.45f,radius*.42f),WHITE);}
 else DrawCircleV(p,std::max(1.f,w*.28f),Color{255,244,207,255});
}
}
