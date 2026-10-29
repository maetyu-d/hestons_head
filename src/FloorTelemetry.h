#pragma once
#include "Factory.h"

namespace aftertaste {
// Read-only presentation data. This layer never advances chemistry or creates food.
struct MaterialView {
 FoodMetrics metrics;
 Compounds compounds{};
 double droplet=90,particle=180,air=0,shell=0,denatured=0,links=0,starch=0;
 bool vapor=false;std::string fraction;
 std::map<std::string,double> ingredients;
 std::vector<std::string> events;
 std::vector<std::pair<double,std::string>> components;
};
MaterialView materialView(const CulinaryBatch& food);
struct FloorSample {Position position;int slot=0;MaterialView material;std::vector<Position> route;uint64_t split=0,sidePart=0;std::vector<uint64_t> parts;};
struct FloorMotion {uint64_t batch=0;Position from,to;int slot=0;MaterialView material;double start=0,duration=.4;};
struct FloorChange {uint64_t batch=0;int tick=0;std::string title;std::vector<std::string> details;MaterialView material;};
class FloorTelemetry {
public:
 std::map<uint64_t,FloorSample> samples;
 std::map<Position,FloorChange> changes;
 std::vector<FloorMotion> motions;
 void clear();
 void observe(const Factory& factory,double now,double duration=.4);
 bool moving(uint64_t batch,double now)const;
private:
 int tick=-1;uint64_t fingerprint=0;
 std::map<Position,std::pair<uint64_t,uint64_t>> inputs;
 std::map<Position,std::pair<uint64_t,MaterialView>> entries;
};
} // namespace aftertaste
