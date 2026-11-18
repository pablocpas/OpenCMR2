// Actual camera update/apply, including saved pitch, terrain and per-record state.
#include "port/types.h"
#include "CameraOperator.h"
#include "CameraRecord.h"
#include "Car.h"
#include "RallyRoute.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <cstdlib>
BYTE g_playerCameraMode[8]={};BYTE g_droneVariant[8]={};int g_physicsTimeStep=65536;
static Car car={};static int scale=50;static bool bank=false;static int drop=100;
RallyRouteNode*g_routeNodes=nullptr;int g_routeNodeCount=0;
int RallyData_GetActiveCarRaceRecordField0(Car*){return 1;}
int RallyData_GetRouteAdditionalState(){return 0;}
bool Track_CameraGroundHeight(FixVector*p,int*y){*y=(bank&&p->z<-65536)?4*65536:0;return true;}
Car*Car_Get(int i){return i==0?&car:nullptr;}
extern "C" int Sys_GetOption(const char*k,int fallback){return !strcmp(k,"video.drone_speed_drop")?drop:!strcmp(k,"video.drone_distance")?scale:!strcmp(k,"video.camera_operator")?1:fallback;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 FixMatrix body={};body.up.y=body.right.z=65536;body.forward.x=-65536;car.pWorld=&body;car.speed=98304;
 FixVector offset={0,98304,425984};CameraRecord record={};g_playerCameraMode[0]=4;g_droneVariant[0]=1;
 RallyRouteNode nodes[6]={};const int coords[6][2]={{0,-10},{0,10},{5,20},{15,30},{25,35},{35,40}};
 for(int i=0;i<6;i++){nodes[i].position.x=coords[i][0]*65536;nodes[i].position.z=coords[i][1]*65536;}g_routeNodes=nodes;g_routeNodeCount=6;
 for(int hz:{25,60,120})for(int pct:{50,60,100,150})for(bool b:{false,true}){
  scale=pct;bank=b;g_physicsTimeStep=int(65536.f*25/hz);CameraOperator_Reset(0);
  for(int i=0;i<hz*10;i++){CameraOperator_Update(&record,&body,&offset,0x2d);CameraOperator_Apply(&record);}
  auto&m=record.matrix;float fx=m.forward.x/65536.f,fy=m.forward.y/65536.f,fz=m.forward.z/65536.f;
  CHECK(fz>.90f);CHECK(std::abs(fx)<.035f);CHECK(m.position.z<=-3.9f*65536);CHECK(fy>-.43f);
  // Perspective point50m ahead stays in front of camera, above the player's center.
  auto screenY=[&](float y,float z) -> float{float dy=y-m.position.y/65536.f,dz=z-m.position.z/65536.f;float depth=dy*fy+dz*fz;if(depth<=0){fprintf(stderr,"point behind camera\n");std::exit(1);}return .5f-.5f*(2.08f*record.zoom/65536.f)*(dy*m.up.y/65536.f+dz*m.up.z/65536.f)/depth;};
  float player=screenY(1,0),road=screenY(0,50);CHECK(player>.60f&&player<.72f);CHECK(road<player-.06f&&road>0);   // margin .10 -> .06: the tuned default drone height (60%) sits the camera lower, so the point 50 m ahead lands relatively higher (measured player .635/.666, road .261-.267 banked, .538-.567 flat)
  if(hz==25)printf("%d %d %g %g %g %g %g %g %g\n",pct,int(b),m.position.x/65536.f,m.position.y/65536.f,m.position.z/65536.f,fx,fy,fz,record.zoom/40960.f);
 }
 // Actual runtime speed-height binding and disable control, not SpeedLook.
 bank=false;scale=50;g_physicsTimeStep=65536;g_routeNodes=nullptr;g_routeNodeCount=0;
 float heights[3];
 for(int amount:{0,100,200}){drop=amount;CameraOperator_Reset(0);for(int i=0;i<300;i++)CameraOperator_Update(&record,&body,&offset,0x2d);CameraOperator_Apply(&record);heights[amount/100]=record.matrix.position.y/65536.f;}
 CHECK(heights[0]-heights[1]>0.15f);CHECK(heights[1]>2.19f);   // tuned default height 60 compresses the range: drop0 2.43, drop100/200 floor at 2.2 (measured); the 2.2 floor literal is unchanged, so the binding still bites and still floors
 CHECK(heights[2]>2.19f&&heights[2]<=heights[1]+1e-4f);   // 200% drops at least as far, never through the 2.2 floor
 puts("actual camera update/apply road-facing pitch and perspective at25/60/120Hz pass");
}
