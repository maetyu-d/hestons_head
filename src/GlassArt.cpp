#include "Theme.h"
#include "GlassArt.h"
#include <algorithm>
#include <cmath>

namespace aftertaste {
namespace {
float length(Vector2 a,Vector2 b){return std::hypot(b.x-a.x,b.y-a.y);}
Vector2 edge(Rectangle r,int dir){auto d=directions[(dir%4+4)%4];return {r.x+r.width*(.5f+d[0]*.5f),r.y+r.height*(.5f+d[1]*.5f)};}
Vector2 rotate(Rectangle r,float x,float y,int dir){for(int n=0;n<(dir%4+4)%4;n++){float z=x;x=1-y;y=z;}return {r.x+x*r.width,r.y+y*r.height};}
}
std::vector<Vector2> glassPath(Rectangle r,MachineKind kind,int inlet,int outlet){
 Vector2 a=edge(r,inlet),b=edge(r,outlet),c={r.x+r.width*.5f,r.y+r.height*.5f};
 if(kind==MachineKind::GlassCoil){std::vector<Vector2> p={a};for(auto q:std::vector<Vector2>{{.17f,.50f},{.25f,.20f},{.75f,.20f},{.75f,.35f},{.25f,.35f},{.25f,.50f},{.75f,.50f},{.75f,.65f},{.25f,.65f},{.25f,.80f},{.75f,.80f},{.86f,.50f}})p.push_back(rotate(r,q.x,q.y,outlet));p.push_back(b);return p;}
 if(kind==MachineKind::GlassValve||((inlet+2)%4)==outlet)return {a,c,b};
 std::vector<Vector2> p;for(int n=0;n<=24;n++){float t=n/24.f,u=1-t;p.push_back({u*u*a.x+2*u*t*c.x+t*t*b.x,u*u*a.y+2*u*t*c.y+t*t*b.y});}return p;
}
Vector2 glassPoint(const std::vector<Vector2>& p,float fraction){
 if(p.empty())return {};if(p.size()==1)return p.front();float total=0;for(size_t n=1;n<p.size();n++)total+=length(p[n-1],p[n]);float target=std::clamp(fraction,0.f,1.f)*total;
 for(size_t n=1;n<p.size();n++){float span=length(p[n-1],p[n]);if(target<=span||n+1==p.size()){float t=span>0?std::clamp(target/span,0.f,1.f):0;return {p[n-1].x+(p[n].x-p[n-1].x)*t,p[n-1].y+(p[n].y-p[n-1].y)*t};}target-=span;}return p.back();
}
void drawGlassWall(const std::vector<Vector2>& p,float diameter,bool ghost,bool mono,bool chemical){
 if(p.size()<2)return;float opacity=ghost?.40f:1.f;Color wall=mono?Color{88,88,88,255}:chemical?Color{158,57,216,255}:Color{86,142,157,255},wash=mono?Color{180,180,180,255}:chemical?Color{215,170,245,255}:Color{192,235,239,255};
 for(size_t n=1;n<p.size();n++)DrawLineEx(p[n-1],p[n],diameter,Fade(wash,(chemical?.30f:.18f)*opacity));
 std::vector<Vector2> left,right,shine;
 for(size_t n=0;n<p.size();n++){auto a=p[n?n-1:n],b=p[n+1<p.size()?n+1:n];float len=length(a,b);Vector2 normal=len>.001f?Vector2{-(b.y-a.y)/len,(b.x-a.x)/len}:Vector2{0,1};left.push_back({p[n].x+normal.x*diameter*.5f,p[n].y+normal.y*diameter*.5f});right.push_back({p[n].x-normal.x*diameter*.5f,p[n].y-normal.y*diameter*.5f});shine.push_back({p[n].x+normal.x*diameter*.30f,p[n].y+normal.y*diameter*.30f});}
 float stroke=std::clamp(diameter*(chemical?.16f:.10f),chemical?1.15f:.8f,chemical?2.2f:1.8f);for(size_t n=1;n<p.size();n++){DrawLineEx(left[n-1],left[n],stroke,Fade(wall,.9f*opacity));DrawLineEx(right[n-1],right[n],stroke,Fade(wall,.75f*opacity));DrawLineEx(shine[n-1],shine[n],stroke,Fade(WHITE,.95f*opacity));}
 if(chemical)for(float t:{.12f,.88f}){auto c=glassPoint(p,t),q=glassPoint(p,std::min(1.f,t+.01f));float len=length(c,q);if(len>.001f){Vector2 normal{-(q.y-c.y)/len*diameter*.58f,(q.x-c.x)/len*diameter*.58f};DrawLineEx({c.x-normal.x,c.y-normal.y},{c.x+normal.x,c.y+normal.y},stroke,Fade(wall,opacity));}}
 for(size_t n:{size_t(0),p.size()-1}){auto a=p[n? n-1:n],b=p[n+1<p.size()?n+1:n];float len=length(a,b);if(len<.001f)continue;Vector2 v={-(b.y-a.y)/len*diameter*.6f,(b.x-a.x)/len*diameter*.6f};DrawLineEx({p[n].x-v.x,p[n].y-v.y},{p[n].x+v.x,p[n].y+v.y},stroke*2,Fade(wall,.8f*opacity));DrawLineEx({p[n].x-v.x*.72f,p[n].y-v.y*.72f},{p[n].x+v.x*.72f,p[n].y+v.y*.72f},stroke,Fade(WHITE,opacity));}
}
} // namespace aftertaste
