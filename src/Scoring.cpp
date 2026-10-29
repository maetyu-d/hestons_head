#include "Model.h"
#include <algorithm>
#include <numeric>

namespace aftertaste {
namespace {template<size_t N> int major(const std::array<double,N>& v){return int(std::max_element(v.begin(),v.end())-v.begin());}}
int Game::feedCount(const std::vector<int>& route)const{int n=0;for(int i:route)if(edible(board[i])&&board[i].delivered>.028)n++;return n;}
Engineering Game::engineeringMetrics(const std::vector<int>& route)const{
 Engineering e;auto& spec=data()["COMMISSIONS"][course];e.need=spec["need"];e.fedNeed=spec["feeds"];e.fed=feedCount(route);std::vector<int> ps;
 for(int i:route)if(edible(board[i]))ps.push_back(i);
 double fouling=0;for(auto& c:board){e.waste+=c.waste;e.energy+=c.energy;e.ruptures+=c.ruptures;fouling+=c.fouling;}
 if(course==0)for(int i:ps)if(board[i].delivered>.028){for(auto p:foods(board[i]))if(p->phase=="pearls"){e.n++;break;}}
 if(course==1)for(int i:ps)if(board[i].delivered>.028&&profile(board[i]).visible>0)e.n++;
 if(course==2)for(size_t j=1;j<ps.size();j++){double t=profile(board[ps[j]]).temp,b=profile(board[ps[j-1]]).temp;if(t*b<-.12&&std::abs(t-b)>1)e.n++;}
 if(course==3){std::set<int> families;for(int i:ps){auto p=profile(board[i]);if(board[i].delivered>.028&&*std::max_element(p.a.begin(),p.a.end())>.20)families.insert(major(p.a));}e.n=int(families.size());}
 if(course==4)for(int i:ps){auto p=profile(board[i]);if(board[i].delivered>.028&&p.chemistry&&p.integrity>.36)e.n++;}
 if(course==5)for(int i:ps){auto p=profile(board[i]);if(board[i].delivered>.028&&p.integrity>.65&&p.scorch<.55)e.n++;}
 if(course==6)e.n=int(runDiscoveries.size());
 e.feedRatio=e.fedNeed?clamp(double(e.fed)/e.fedNeed,0,1):1;e.purity=clamp(1-e.waste/(1.5+ps.size()*.3)-fouling*.018,.12,1);e.score=clamp(double(e.n)/e.need,0,1);return e;
}
double textureFit(const std::string& a,const std::string& w){if(a==w)return 1;if((a=="pearls"&&w=="gel")||(a=="crystal"&&w=="gel")||(a=="emulsion"&&w=="liquid"))return .85;if((a=="foam"&&w=="emulsion")||(a=="liquid"&&w=="foam"))return .5;return .26;}
Vec Game::expectation(const Cell& c)const{
 auto v=profile(c);if(!v.has)return {};Vec f=v.face?taste(*v.face):v.f;
 if(v.visible==1)f={2.9,.1,.1,.1,0};if(v.visible==2)f={.1,.2,2.65,.9,0};if(v.visible==3)f={.6,.1,.3,2,.45};
 Vec base=data()["MENU_EXPECT"][menu].get<Vec>(),result{};for(int i=0;i<5;i++)result[i]=.59*f[i]+.41*base[i];
 if(sound=="sea"){result[2]+=.24;result[3]+=.07;}if(sound=="nursery")result[0]+=.26;if(sound=="hearth"){result[3]+=.18;result[4]+=.16;}if(sound=="forest")result[4]+=.21;return result;
}
double Game::affinity(const Profile& p)const{double target=0;if(sound=="sea")target=p.a[0]+p.f[2]*.3;if(sound=="nursery")target=p.a[1]+p.f[0]*.28;if(sound=="hearth")target=p.a[2]+p.roast*.65;if(sound=="forest")target=p.a[3]+p.a[2]*.35;return sound=="silence"?.25:clamp(target/2.4,0,1);}
int Game::contractCount(const std::vector<int>& route,const std::vector<Bite>& bites)const{
 std::string kind=data()["CONTRACTS"][diner.contract]["kind"];std::vector<Profile> ps;for(int i:route)ps.push_back(profile(board[i]));int n=0;
 if(kind=="illusion")for(auto& b:bites)if(profile(board[b.i]).visible>0&&b.surprise>.40&&b.d>.85&&b.quality>.43)n++;
 if(kind=="thermal")for(size_t j=1;j<ps.size();j++)if(ps[j].temp*ps[j-1].temp<-.16&&std::abs(ps[j].temp-ps[j-1].temp)>1.15)n++;
 if(kind=="aroma")for(int i:route)if(*std::max_element(board[i].aroma.begin(),board[i].aroma.end())>.29)n++;
 if(kind=="react")for(auto& p:ps)if(p.chemistry)n++;
 if(kind=="layers")for(auto& p:ps)if(p.layers>=3)n++;
 if(kind=="clean")for(auto& p:ps)if((p.phase=="gel"||p.phase=="pearls"||p.phase=="crystal"||p.phase=="crisp"||p.phase=="foam"||p.phase=="emulsion")&&p.scorch<.35&&p.split<.45)n++;
 return n;
}
struct Mouth {double aromaHold=0,aftertaste=0,coating=0,heat=0;};
struct Oral {std::vector<std::string> details;double release=0,aroma=0,thermal=0,quality=0,aftertaste=0,coating=0;};
static Oral mouthTransition(const Cell& c,const Profile& p,Mouth& state){
 Oral o;double delay=0;o.thermal=p.temp;
 if(c.delivered>.028)o.details.push_back("SOURCE-FED");if(p.reveal>.38)o.details.push_back("LAYER REVEAL");
 if(p.phase=="pearls"||p.phase=="crisp"){o.details.push_back(p.phase=="pearls"?"BURSTING PEARLS":"SHELL FRACTURE");o.quality+=.055;o.release+=.24;}
 if(p.phase=="gel"&&p.temp>-.45){o.details.push_back("GEL MELTS");o.quality+=.03;o.thermal*=.77;o.release+=.12;}
 if(p.phase=="crystal"){o.details.push_back("COLD DISSOLUTION");o.thermal*=.63;o.quality+=.03;}
 if(p.phase=="foam"){o.details.push_back("FOAM COLLAPSE");o.release+=.16;}
 if(p.phase=="emulsion"){o.details.push_back("CREAMY COATING");state.coating+=.25;}
 if(p.roast>.35){o.details.push_back("ROASTED FINISH");delay=p.roast*.27;}
 if(c.burst>.1||c.gas>.3){o.details.push_back("DELAYED AROMA");o.release+=clamp(c.gas*.15+c.burst*.3,0,.55);}
 o.release+=clamp(*std::max_element(p.a.begin(),p.a.end())*.07,0,.35);double incoming=state.aromaHold;state.aromaHold=clamp(state.aromaHold*.46+o.release*.56+delay,0,1.8);state.coating=clamp(state.coating*.75,0,1.5);
 if(p.f[1]>1.1){o.details.push_back("PALATE REFRESH");state.coating*=.52;state.aftertaste*=.60;}else if(state.aftertaste>.38)o.details.push_back("LINGERING AFTERTASTE");
 state.aftertaste=clamp(state.aftertaste*.62+p.f[4]*.12+p.roast*.16,0,2);state.heat=state.heat*.40+o.thermal*.60;if(o.details.empty())o.details.push_back("PALATE ADAPTS");o.aroma=incoming+o.release;o.aftertaste=state.aftertaste;o.coating=state.coating;return o;
}
Report Game::evaluate(const std::vector<int>& route)const{
 Report out;double trust=1,flow=0,structure=0,chemistry=0;Vec palate{};std::optional<Vec> learned;std::optional<Profile> previous;Mouth mouth;
 for(size_t k=0;k<route.size()&&k<6;k++){
  auto& c=board[route[k]];auto p=profile(c);auto& target=data()["COURSES"][course]["targets"][k];if(!p.has)continue;
  auto oral=mouthTransition(c,p,mouth);Bite b;b.i=route[k];b.k=int(k);
  for(int j=0;j<5;j++)b.felt[j]=clamp((p.f[j]+(j==4?oral.aftertaste*.16:0))*diner.sensitivity[j]*(1-clamp(palate[j]*.11,0,.34))*(j==0?1-mouth.coating*.045:1),0,5);
  double difference=0;for(int j=0;j<5;j++)difference+=std::abs(target["f"][j].get<double>()-b.felt[j])*(j==4?.85:1);b.match=std::exp(-.205*difference);
  double tempAtBite=oral.thermal*std::pow(.95,double(k));b.thermal=std::exp(-.35*std::abs(target["t"].get<double>()-tempAtBite));b.texture=textureFit(p.phase,target["phase"]);
  b.condition=clamp(.98-p.scorch*.4-p.split*.29-(1-p.integrity)*.24,.22,1);b.delight=clamp(1+.09*b.felt[diner.like]-.095*b.felt[diner.avoid]+.035*(p.phase==diner.texture?1:0),.62,1.16);
  b.quality=clamp((.44*b.match+.18*b.thermal+.18*b.texture+.20*b.condition+oral.quality)*b.delight,0,1.12);
  auto named=expectation(c);for(int j=0;j<5;j++)b.expected[j]=learned?trust*.78*named[j]+(1-trust*.78)*(*learned)[j]:named[j];
  double sum=0;for(int j=0;j<5;j++)sum+=std::pow(b.expected[j]-b.felt[j],2);b.d=std::sqrt(sum*.55);b.surprise=std::exp(-std::pow((b.d-diner.novelty)/1.03,2));b.context=affinity(p);double shock=clamp(b.d-diner.novelty-.5,0,3);b.fed=clamp(c.delivered/.055,0,1);
  double edibleScore=100*b.quality*(.75+.16*b.surprise+.09*b.context)*(.92+.08*trust)+p.reveal*4+(c.burst?.8:0)+oral.aroma*2;b.score=edibleScore*(course==6?1:(.79+.21*b.fed));
  b.trust=trust;trust=clamp(trust+.085*(b.quality-.50)-shock*.10,.36,1);b.trustAfter=trust;
  if(previous){auto& a=*previous;if(p.temp*a.temp<-.18&&std::abs(p.temp-a.temp)>1.1)flow+=1.9;if(a.f[3]+a.f[4]>1.8&&p.f[1]>.9)flow+=1.3;if(p.phase!=a.phase)flow+=.46;if(a.f[2]>1.45&&p.f[0]>1.4)flow+=.54;if(major(p.a)!=major(a.a))flow+=.3;}
  structure+=p.layers>=2?p.reveal*.75+.3:0;chemistry+=p.chemistry?.85:0;
  b.phase=p.phase;b.details=oral.details;b.release=oral.release;b.aftertaste=oral.aftertaste;out.bites.push_back(b);
  for(int j=0;j<5;j++)palate[j]=clamp(palate[j]*diner.memory+b.felt[j]*.31*(j==4?1.25:1),0,4);
  if(p.f[1]>1.1)for(int j:{0,2,3,4})palate[j]*=.69;if(p.f[4]>1.2)palate[4]=clamp(palate[4]+.16,0,4);
  if(learned)for(int j=0;j<5;j++)(*learned)[j]=.63*(*learned)[j]+.37*b.felt[j];else learned=b.felt;previous=p;
 }
 for(auto& b:out.bites)out.avg+=b.score;out.avg/=std::max(size_t(1),out.bites.size());out.engineering=engineeringMetrics(route);auto& e=out.engineering;out.contractN=contractCount(route,out.bites);
 out.goal=17*e.score;out.contract=6*clamp(double(out.contractN)/data()["CONTRACTS"][diner.contract]["need"].get<double>(),0,1);out.transport=13*e.feedRatio;
 out.flow=clamp(flow,0,5);out.chemistry=clamp(chemistry,0,4);out.structure=clamp(structure,0,3);
 out.wastePenalty=clamp(e.waste*12,0,17);out.energyPenalty=course==5?clamp(e.energy*.75,0,10):clamp(e.energy*.14,0,3);out.rupturePenalty=clamp(e.ruptures*(course==4?4:1.8),0,12);out.impurityPenalty=clamp((1-e.purity)*12,0,10);
 out.final=int(std::floor(clamp(out.avg*.64+out.goal+out.contract+out.transport+out.flow+out.chemistry+out.structure-out.wastePenalty-out.energyPenalty-out.rupturePenalty-out.impurityPenalty,0,100)+.5));out.goalN=e.n;out.trust=trust;
 out.phrase=out.final>=86?"A BEAUTIFUL CONTRADICTION.":out.final>=73?"THE DINER IS TRANSPORTED.":out.final>=60?"A PROMISING EXPERIMENT.":"AN UNFINISHED DISH.";return out;
}
void Game::serve(){
 if(mode=="tasting"){nextTaste();return;}if(path.size()!=6){announce("SIX CONNECTED EDIBLE BITES REQUIRED BEFORE SERVICE.");return;}
 if(course!=6&&feedCount(path)<data()["COMMISSIONS"][course]["feeds"].get<int>()){announce("FEED THE TASTING ROUTE FROM RESERVOIRS FIRST ("+std::to_string(feedCount(path))+" / "+data()["COMMISSIONS"][course]["feeds"].dump()+").");return;}
 report=evaluate();tasteIndex=0;mode="tasting";lift=-1;announce("SERVICE BEGINS - CONSUME SIX BITES TO REVEAL THE VERDICT.");
}
void Game::nextTaste(){
 if(!report||tasteIndex<0||tasteIndex>=6)return;tasteIndex++;auto& bite=report->bites[tasteIndex-1];selected=bite.i;
 if(tasteIndex==6){auto key=std::to_string(course)+"-"+std::to_string(dinerNo);highScores[key]=std::max(highScores[key],report->final);savePersistent();announce("TASTING COMPLETE / "+report->phrase+" / TRUST "+std::to_string(int(std::round(report->trust*100)))+"%");}
 else{std::string details;for(auto& s:bite.details){if(!details.empty())details+=" / ";details+=s;}announce("BITE "+std::to_string(tasteIndex)+" / 6 / "+details+" / TRUST "+std::to_string(int(std::round(bite.trustAfter*100)))+"%");}
}
} // namespace aftertaste
