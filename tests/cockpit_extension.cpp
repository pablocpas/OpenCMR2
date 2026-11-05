#include "CockpitExtension.h"
#include <cstdio>
#include <cstdlib>
static void need(bool c,const char*s){if(!c){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
int main(){
 CockpitExtensionProfile p={{.61f,-.015f,-.62f},{.50f,-.465f,-.644f},{.38f,-.465f,-.108f},{{.49f,.806f},{.529f,.806f},{.533f,.841f},{.533f,.841f}},.588f};
 std::vector<GfxVertex>v;CockpitExtension_Append(v,p,-.859f);
 need(v.size()==36,"wall+floor use three bounded source-patch repeats");
 bool rear=false,uvRange=false;float oldStart=0;
 for(size_t i=0;i<v.size();i++){
  auto&w=v[i];need(std::isfinite(w.x)&&std::isfinite(w.y)&&std::isfinite(w.z),"finite geometry");
  need(w.x>=-.8591f&&w.x<=.611f,"positions stay within actual source edge and requested rear plane");
  need(w.u0>=.4899f&&w.u0<=.5331f&&w.v0>=.8059f&&w.v0<=.8411f,"UV stays in source atlas patch");
  if(std::abs(w.x+.859f)<.0001)rear=true;
  if(std::abs(w.u0-.49f)<.001)uvRange=true;
 }
 need(rear&&uvRange,"reaches rear plane and carries real patch UVs");
 need(v[0].u0==v[12].u0&&v[12].u0==v[24].u0,"UV restarts for each repeat, not atlas wrap or single colour");
 need(v[1].u0>v[0].u0&&v[25].u0>v[24].u0,"full/partial cells span the source patch");
 need(std::abs(v[1].x-v[12].x)<.0001f,"wall cells join without geometry gaps");
 v.clear();CockpitExtension_Append(v,p,1.f);need(v.empty(),"no reversed extension");
 puts("PASS actual extension generator: passenger wall/floor, connected repeats, bounded UV/source geometry");
}
