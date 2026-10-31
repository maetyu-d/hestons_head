#include "Interface.h"
#include <cstdlib>
#include <iostream>

static std::filesystem::path storagePath(){
 if(const char* custom=std::getenv("AFTERTASTE_DATA_DIR"))return std::filesystem::path(custom)/"notebook.json";
#ifdef _WIN32
 const char* root=std::getenv("APPDATA");return (root?std::filesystem::path(root):std::filesystem::temp_directory_path())/"Aftertaste"/"notebook.json";
#elif defined(__APPLE__)
 const char* root=std::getenv("HOME");return (root?std::filesystem::path(root):std::filesystem::temp_directory_path())/"Library"/"Application Support"/"Aftertaste"/"notebook.json";
#else
 if(const char* root=std::getenv("XDG_DATA_HOME"))return std::filesystem::path(root)/"aftertaste"/"notebook.json";
 const char* root=std::getenv("HOME");return (root?std::filesystem::path(root):std::filesystem::temp_directory_path())/".local"/"share"/"aftertaste"/"notebook.json";
#endif
}
int main(int argc,char** argv){
 std::string screenshot;int windowWidth=1220,windowHeight=810,factoryTicks=0,machineDirection=0;bool glassCheck=false,machineCheck=false,check=false,flowCheck=false,closeup=false,fresh=false,smoke=false;
 for(int i=1;i<argc;i++){
  std::string arg=argv[i];
  if(arg=="--screenshot"&&i+1<argc)screenshot=argv[++i];
  else if(arg=="--size"&&i+1<argc){std::string size=argv[++i];auto split=size.find('x');if(split!=std::string::npos){windowWidth=std::stoi(size.substr(0,split));windowHeight=std::stoi(size.substr(split+1));}}
  else if(arg=="--floor-test"||arg=="--one-view-test"||arg=="--ui-test"||arg=="--factory-test"||arg=="--studio-test")check=true;
  else if(arg=="--glass-test")glassCheck=true;
  else if(arg=="--machine-test")machineCheck=true;
  else if(arg=="--machine-direction"&&i+1<argc)machineDirection=(std::stoi(argv[++i])%4+4)%4;
  else if(arg=="--flow-test")flowCheck=true;
  else if(arg=="--flow-closeup"){flowCheck=true;closeup=true;}
  else if(arg=="--fresh-factory")fresh=true;
  else if((arg=="--factory-ticks"||arg=="--wait")&&i+1<argc)factoryTicks=std::stoi(argv[++i]);
  else if(arg=="--smoke")smoke=true;
  else if(arg=="--course"&&i+1<argc)++i;
  else if(arg=="--lab"||arg=="--studio"){} // Older launch aliases all open the same workspace.
  else if(arg=="--help"){std::cout<<"Aftertaste / factory-floor molecular sandbox\n--floor-test --glass-test --machine-test --machine-direction 0..3 --flow-test --flow-closeup --fresh-factory --factory-ticks N --size WIDTHxHEIGHT --screenshot PATH\nAFTERTASTE_DATA_DIR overrides the save directory.\n";return 0;}
 }
 SetConfigFlags(FLAG_WINDOW_RESIZABLE|FLAG_WINDOW_HIGHDPI|FLAG_VSYNC_HINT|FLAG_MSAA_4X_HINT);SetTraceLogLevel(LOG_WARNING);
 InitWindow(windowWidth,windowHeight,"AFTERTASTE / MOLECULAR WORKS");SetWindowMinSize(820,620);
 const int monitor=GetCurrentMonitor(),fittedWidth=std::min(windowWidth,std::max(820,GetMonitorWidth(monitor)-60)),fittedHeight=std::min(windowHeight,std::max(620,GetMonitorHeight(monitor)-90));
 if(fittedWidth!=windowWidth||fittedHeight!=windowHeight)SetWindowSize(fittedWidth,fittedHeight);const auto origin=GetMonitorPosition(monitor);SetWindowPosition(int(origin.x+(GetMonitorWidth(monitor)-fittedWidth)/2),int(origin.y+std::max(52,(GetMonitorHeight(monitor)-fittedHeight)/2)));
 SetExitKey(KEY_NULL);SetTargetFPS(60);int exitCode=0;
 try{
  auto storage=(check||flowCheck||machineCheck||glassCheck)&&!std::getenv("AFTERTASTE_DATA_DIR")?std::filesystem::temp_directory_path()/"aftertaste-one-view-test"/"notebook.json":storagePath();
  aftertaste::Interface ui(storage);if(check||flowCheck||machineCheck||glassCheck)ui.factory.reset();if(fresh)ui.factory.resetDemo();for(int n=0;n<factoryTicks;n++)ui.factory.step();
  int frames=0;while(!WindowShouldClose()){
   ui.update();BeginDrawing();ui.draw();EndDrawing();frames++;
   if(check)ui.checkWorkspaceStep(frames);
   if(glassCheck)ui.checkGlassStep(frames);
   if(glassCheck&&frames==16)std::cout<<"PASS glass lab: rendered construction plan, connected glass routes, real stopcock backpressure, burette source, heated/cooled food, beaker recording and serving, no separate plate\n";
   if(machineCheck)ui.checkMachineStep(frames,machineDirection);
   if(machineCheck&&frames==5)std::cout<<"PASS machine recognition: 94 equipment types, 752 monochrome render checks across two sizes and four rotations, translucent previews, idle/working mechanisms, bounded food windows, unchanged model\n";
   if(flowCheck)ui.checkFlowStep(frames,closeup);
   if(flowCheck&&frames==60)std::cout<<"PASS native floor flow: live transfers, molecular structures, measured reactions, actual ingredient labels and Flow toggle preserve the world\n";
   if(check&&frames==23)std::cout<<"PASS factory-floor invention: measured feeder batches, actual mixer/doser/homogeniser, tasting, naming, physical line copying and repeated production; no separate test plate\n";
   if(!screenshot.empty()&&frames==(glassCheck?20:flowCheck?62:check?25:6)){Image frame=LoadImageFromScreen();bool saved=ExportImage(frame,screenshot.c_str());UnloadImage(frame);if(!saved){std::cerr<<"Could not export screenshot: "<<screenshot<<'\n';exitCode=1;}break;}
   if(glassCheck&&frames==22)break;if(machineCheck&&frames==8)break;if(flowCheck&&frames==64)break;if(check&&frames==26)break;if(smoke&&frames==12)break;
  }
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';exitCode=1;}
 CloseWindow();return exitCode;
}
