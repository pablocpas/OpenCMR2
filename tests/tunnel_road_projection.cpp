// Route -> production fixed-point view projection -> runtime focus feed.
// CPU integration test, not native Metal pixel acceptance.
#include "Graphics.h"
#include "TunnelRoadProjection.h"
#include "CameraOperator.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern const float g_floatHalf = 0.5f;
double CGraphics::m_oneOver65536 = 1.0 / 65536.0;
double CGraphics::m_65536 = 65536.0;
Graphics *g_pGraphics;
static Graphics gfx;
static D3DTextureManager tex;
D3DTextureManager *CGraphics::m_pTextureManager = &tex;
static void need(bool ok,const char *s){if(!ok){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
static bool close(float a,float b){return std::abs(a-b)<.002f;}
int main(){
 gfx.resX=1920;gfx.resY=1080;g_pGraphics=&gfx;
 tex.fixedProjection[0]=65536;tex.fixedProjection[5]=65536;
 tex.fixedProjection[10]=65536;tex.fixedProjection[11]=65536;
 SceneNode view={};FixMatrix_Identity(&view.world);
 float u,v;FixVector p={0,0,30*65536};
 need(TunnelRoad_ProjectPoint(&p,&view,1920,1080,&u,&v)&&close(u,.5f)&&close(v,.5f),"16.16 centre is centre, not max-clamped");
 p={6*65536,-6*65536,30*65536};
 need(TunnelRoad_ProjectPoint(&p,&view,1920,1080,&u,&v)&&close(u,.6f)&&close(v,.6f),"right/below point normalizes to .6,.6 v-down");
 p={-6*65536,6*65536,30*65536};
 need(TunnelRoad_ProjectPoint(&p,&view,1920,1080,&u,&v)&&close(u,.4f)&&close(v,.4f),"left/above point normalizes to .4,.4");
 p={0,0,-65536};need(!TunnelRoad_ProjectPoint(&p,&view,1920,1080,&u,&v)&&u==.5f&&v==.5f,"behind camera returns centre");
 float last=.5f;
 for(int n=-20;n<=20;n++){
  // A family of left/straight/right road bends. This is the real preview,
  // not a supplied UV. At 30 units the far road point varies continuously.
  CameraOperator::V3 nodes[]={{0,0,0},{0,0,12},{n*.25f,0,30},{n*.5f,0,60}};
  auto road=CameraOperator::PreviewRoad({0,0,0},1,4,false,[&](int i){return nodes[i];});
  need(road.valid,"route preview valid");
  p={(int)std::lround(road.far.x*65536),-65536,(int)std::lround(road.far.z*65536)};
  need(TunnelRoad_ProjectPoint(&p,&view,1920,1080,&u,&v),"bend projects");
  need(u>.3f&&u<.7f&&v>.5f&&v<.55f,"road focus remains on road, not clamp edge");
  if(n>-20)need(u>=last&&u-last<.01f,"smooth bend sweep does not stick or jump");last=u;
 }
 // Live view and FOV changes must change the projection, not a stale feed.
 p={6*65536,0,30*65536};tex.fixedProjection[0]=32768;
 TunnelRoad_ProjectPoint(&p,&view,1920,1080,&u,&v);need(close(u,.55f),"wide projection applies");
 view.world.position.x=6*65536;
 TunnelRoad_ProjectPoint(&p,&view,1920,1080,&u,&v);need(close(u,.5f),"current view translation applies");
 puts("PASS route -> production 16.16 projection -> normalized focus, bend continuity, current FOV/view");
}
