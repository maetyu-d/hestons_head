#pragma once
#include "Factory.h"
#include "FloorTelemetry.h"
#include "raylib.h"
#include <functional>

namespace aftertaste {
class Interface {
public:
 explicit Interface(const std::filesystem::path& storage);
 ~Interface();
 Factory factory;
 void checkFlowStep(int frame,bool closeup=false);
 void checkMachineStep(int frame,int direction=0);
 void checkGlassStep(int frame);
 void update();void draw();void checkWorkspaceStep(int step);
private:
 struct Hit {Rectangle bounds;std::function<void()> action;bool enabled;std::string tip,label;};
 std::vector<Hit> hits;
 Font regular{},bold{},mono{};
 bool ownsRegular=false,ownsBold=false,ownsMono=false,clipping=false;
 Rectangle clipRect{},factoryView{},factoryMiniMap{},inspectorRect{},inspectorBody{},recipeBody{};
 std::array<Rectangle,2> libraryBodies{};
 std::array<float,2> libraryScroll{},libraryHeights{};
 float uiScale=1,tooltipTime=0,inspectorScroll=0,inspectorHeight=0,recipeScroll=0,recipeHeight=0,modalScroll=0,modalContentHeight=0;
 Vector2 lastMouse{};
 std::string tooltip,librarySearch,sourceIngredient="tomato",doseText="100.00",dishName="Untitled dish",planName;
 int editFocus=0,batchSlot=0,componentIndex=0,latticeSelected=-1,planRenameIndex=-1;
 FloorTelemetry floorTelemetry;
 bool floorFlow=true;
 bool initialFraming=true,sourceBrushConfigured=false;
 uint64_t referenceRecipe=0;
 double sourceDose=100;
 Vector2 factoryCamera{-.5f,2.5f},factoryPanMouse{},factoryPanWorld{},factoryZoomAnchorMouse{},factoryZoomAnchorWorld{};
 Position factorySelected{-3,6},factoryHovered{},factoryDragStart{},factoryLastPaint{};
 MachineKind factoryTool=MachineKind::Belt;
 int factoryDirection=0,factoryTab=1,factoryOverlay=0,factorySpeed=1;
 float factoryZoom=1,factoryZoomTarget=1,factoryAccumulator=0,factorySaveTimer=0;
 int factoryPanButton=-1;
 bool factoryZooming=false,factoryPanning=false,factoryPanMoved=false,factoryPainting=false,factoryCopying=false,factorySelecting=false,factoryErasing=false;
 std::optional<Blueprint> factoryStamp;
 void text(const std::string& s,float x,float y,float size,Color color,bool strong=false,bool fixed=false)const;
 float width(const std::string& s,float size,bool strong=false,bool fixed=false)const;
 std::string fit(const std::string& s,float maxWidth,float size,bool strong=false,bool fixed=false)const;
 float paragraph(const std::string& s,float x,float y,float maxWidth,float size,Color color,bool strong=false)const;
 void button(Rectangle r,const std::string& s,std::function<void()> action,bool active=false,bool enabled=true,const std::string& tip="",Color fill={0,0,0,0},float size=11);
 void hit(Rectangle r,std::function<void()> action,bool enabled=true,const std::string& tip="");
 Vector2 pointer()const;float viewWidth()const;float viewHeight()const;
 void beginClip(Rectangle r);void endClip();void drawTooltip();void editText(std::string& value,size_t limit);
 void stepFactory();void updateFactory();void advanceFactory();void drawFactory();void drawFactoryMap();void drawFactoryOverlay();
 void drawLibrary(Rectangle r);void drawInspector(Rectangle r);
 Position factoryPosition(Vector2 point)const;Rectangle factoryTile(Position p)const;
 void paintFactory(Position p);void fitFactory();void homeFactory();void selectMachine(Position p);
 const Batch* inspectedBatch()const;bool commitDose();void setSource();void armSource(MachineKind kind);
 void recordSelectedDish(uint64_t expectedBatch=0);void buildDishLine(uint64_t id);
 void saveFactoryCheckpoint();void loadFactoryCheckpoint();
};
} // namespace aftertaste
