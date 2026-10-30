#include "Theme.h"
#include "Interface.h"
#include "rlgl.h"
#include <algorithm>
#include <sstream>
namespace aftertaste {
namespace {
const Color paper={246,249,255,255},ink={22,41,75,255},line={183,205,236,255},white={255,255,255,255};
Color blend(Color a,Color b,double f){return {static_cast<unsigned char>(std::round(a.r*(1-f)+b.r*f)),static_cast<unsigned char>(std::round(a.g*(1-f)+b.g*f)),static_cast<unsigned char>(std::round(a.b*(1-f)+b.b*f)),255};}
void border(Rectangle r,Color c,float thickness=1){DrawRectangleLinesEx(r,thickness,c);}
std::string firstExisting(const std::vector<std::string>& paths){for(auto& path:paths)if(std::filesystem::exists(path))return path;return {};}
}
Interface::Interface(const std::filesystem::path& storage){
 std::vector<int> codepoints;for(int c=32;c<=255;c++)codepoints.push_back(c);for(int c:{0x2013,0x2014,0x2019,0x2039,0x203a,0x2190,0x2191,0x2192,0x2193,0x2212,0x221e,0x25b2,0x25bc,0x2713,0x394})codepoints.push_back(c);
 auto regularPath=firstExisting({"/System/Library/Fonts/Supplemental/Arial.ttf","C:/Windows/Fonts/arial.ttf","/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf","/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"});
 auto boldPath=firstExisting({"/System/Library/Fonts/Supplemental/Arial Bold.ttf","C:/Windows/Fonts/arialbd.ttf","/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf","/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf"});
 auto monoPath=firstExisting({"/System/Library/Fonts/Monaco.ttf","C:/Windows/Fonts/consola.ttf","/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf","/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf"});
 regular=regularPath.empty()?GetFontDefault():LoadFontEx(regularPath.c_str(),72,codepoints.data(),int(codepoints.size()));ownsRegular=!regularPath.empty();bold=boldPath.empty()?GetFontDefault():LoadFontEx(boldPath.c_str(),72,codepoints.data(),int(codepoints.size()));ownsBold=!boldPath.empty();mono=monoPath.empty()?GetFontDefault():LoadFontEx(monoPath.c_str(),72,codepoints.data(),int(codepoints.size()));ownsMono=!monoPath.empty();
 for(auto f:{regular,bold,mono})SetTextureFilter(f.texture,TEXTURE_FILTER_BILINEAR);
 lastMouse=pointer();
 factory.storageFile=storage.parent_path()/"factory.json";if(!factory.load())factory.notice="Factory save could not be read. The sample sandbox is available; the saved file is unchanged.";
}
Interface::~Interface(){if(factory.dirty)factory.save();if(ownsRegular)UnloadFont(regular);if(ownsBold)UnloadFont(bold);if(ownsMono)UnloadFont(mono);}
Vector2 Interface::pointer()const{auto p=GetMousePosition();return {p.x/uiScale,p.y/uiScale};}
float Interface::viewWidth()const{return GetScreenWidth()/uiScale;}
float Interface::viewHeight()const{return GetScreenHeight()/uiScale;}
void Interface::beginClip(Rectangle r){clipRect=r;clipping=true;BeginScissorMode(int(r.x*uiScale),int(r.y*uiScale),int(r.width*uiScale),int(r.height*uiScale));}
void Interface::endClip(){EndScissorMode();clipping=false;}
void Interface::update(){uiScale=std::clamp(std::min(GetScreenWidth()/1360.f,GetScreenHeight()/900.f),1.f,1.5f);updateFactory();}
void Interface::draw(){drawFactory();}
void Interface::drawTooltip(){
 if(tooltip.empty()||tooltipTime<.7f||factoryOverlay)return;float w=std::min(370.f,viewWidth()-30);std::istringstream words(tooltip);std::string word,row;int rows=1;
 while(words>>word){auto next=row.empty()?word:row+" "+word;if(width(next,12)>w-22){rows++;row=word;}else row=next;}
 float h=rows*18.f+18;auto m=pointer();Rectangle r={std::clamp(m.x+12,10.f,viewWidth()-w-10),std::clamp(m.y+20,10.f,viewHeight()-h-10),w,h};DrawRectangleRec(r,{255,255,247,255});border(r,ink);paragraph(tooltip,r.x+10,r.y+9,w-20,12,ink);
}
void Interface::editText(std::string& value,size_t limit){
 bool command=IsKeyDown(KEY_LEFT_SUPER)||IsKeyDown(KEY_LEFT_CONTROL);if(command&&IsKeyPressed(KEY_A))value.clear();
 if(IsKeyPressed(KEY_BACKSPACE)||IsKeyPressedRepeat(KEY_BACKSPACE)){if(!value.empty()){size_t n=value.size()-1;while(n>0&&(static_cast<unsigned char>(value[n])&0xc0)==0x80)n--;value.resize(n);}}
 if(command&&IsKeyPressed(KEY_V)){const char* raw=GetClipboardText();if(raw){std::string clip=raw;if(value.size()+clip.size()<=limit)value+=clip;}return;}
 for(int c=GetCharPressed();c;c=GetCharPressed())if(c>=32){int count=0;const char* bytes=CodepointToUTF8(c,&count);if(value.size()+count<=limit)value.append(bytes,size_t(count));}
}
void Interface::text(const std::string& s,float x,float y,float size,Color color,bool strong,bool fixed)const{DrawTextEx(fixed?mono:strong?bold:regular,s.c_str(),{x,y},size,0,color);}
float Interface::width(const std::string& s,float size,bool strong,bool fixed)const{return MeasureTextEx(fixed?mono:strong?bold:regular,s.c_str(),size,0).x;}
std::string Interface::fit(const std::string& s,float maxWidth,float size,bool strong,bool fixed)const{if(width(s,size,strong,fixed)<=maxWidth)return s;auto t=s;while(!t.empty()&&width(t+"...",size,strong,fixed)>maxWidth){size_t pos=t.size()-1;while(pos>0&&(static_cast<unsigned char>(t[pos])&0xc0)==0x80)--pos;t.resize(pos);}return t+"...";}
float Interface::paragraph(const std::string& s,float x,float y,float maxWidth,float size,Color color,bool strong)const{
 std::istringstream words(s);std::string word,row;float start=y;
 while(words>>word){auto test=row.empty()?word:row+" "+word;if(!row.empty()&&width(test,size,strong)>maxWidth){text(row,x,y,size,color,strong);y+=size*1.48f;row=word;}else row=test;}
 if(!row.empty()){text(row,x,y,size,color,strong);y+=size*1.48f;}return y-start;
}
void Interface::hit(Rectangle r,std::function<void()> action,bool enabled,const std::string& tip){if(clipping){if(!CheckCollisionRecs(r,clipRect))return;r=GetCollisionRec(r,clipRect);}hits.push_back({r,std::move(action),enabled,tip,{}});}
void Interface::button(Rectangle r,const std::string& s,std::function<void()> action,bool active,bool enabled,const std::string& tip,Color fill,float size){
 bool over=CheckCollisionPointRec(pointer(),r);Color bg=fill.a?fill:(active?theme::Blue:white);bool dark=(.2126*bg.r+.7152*bg.g+.0722*bg.b)/255<.52;Color fg=dark?white:ink;
 if(!enabled){bg=blend(bg,paper,.58);fg=blend(fg,paper,.45);}else if(over)bg=blend(bg,active?white:ink,.06);
 DrawRectangleRounded(r,.13f,6,bg);border(r,active?theme::Blue:line);auto label=fit(s,r.width-12,size,true);text(label,r.x+(r.width-width(label,size,true))*.5f,r.y+(r.height-size)*.5f,size,fg,true);
 size_t before=hits.size();hit(r,std::move(action),enabled,tip);if(hits.size()>before)hits.back().label=s;
}
} // namespace aftertaste
