#include "port/types.h"
#include "CameraOperator.h"
#include "CameraRecord.h"
#include "Car.h"
#include "port/sys.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <string>
extern void Scene_InitFixedMathTables();
extern void ChaseCamera_Update(CameraRecord*,FixMatrix*,int);
extern BYTE g_playerCameraMode[8];
extern BYTE g_droneVariant[8];
extern FixVector g_chaseCameraOffsets[6],g_chaseCameraViewOffsets[6];
extern int g_chaseCameraLag[4],g_physicsTimeStep;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 auto old=std::filesystem::current_path();auto tmp=std::filesystem::temp_directory_path()/("cmr-operator-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(tmp);std::filesystem::current_path(tmp);std::ofstream("opencmr2.ini") << "[video]\n";
 Scene_InitFixedMathTables();
 Car car={};g_cars[0]=&car;car.surfaceNoise=65536;car.speed=0;
 FixMatrix body={};body.right.z=body.up.y=65536;body.forward.x=-65536;car.pWorld=&body;
 CameraRecord record={};record.index=record.view=record.car=0;
 g_playerCameraMode[0]=4;g_droneVariant[0]=1;g_physicsTimeStep=65536;g_chaseCameraLag[0]=0xe0000;
 Sys_SetOption("video.camera_effects",0);Sys_SetOption("video.camera_operator",1);
 g_chaseCameraOffsets[2]={0,65536,0};g_chaseCameraViewOffsets[2]={65536,0,0};
 ChaseCamera_Update(&record,&body,0);
 CHECK(g_chaseCameraViewOffsets[2].x!=0); // original basis spring no longer bypassed
 // Tuned default distance 150 stands the drone farther back than the old 50 (measured -17.55).
 CHECK(record.matrix.position.z < -16.5*65536 && record.matrix.position.z > -18.5*65536);
 CHECK(std::abs(record.matrix.position.x)<65536/4);
 CHECK(record.matrix.position.y>4*65536 && record.matrix.position.y<7*65536);
 CHECK(record.matrix.forward.z>59000); // forward road, not overhead
 CHECK(record.zoom>0xa000);
 CHECK(g_chaseCameraLag[0]==0xe0000); // never mutate saved spring
 // Actual built camera must now follow behind a turn within about two degrees,
 // and zoom must survive BuildMatrix's original default assignment.
 body.right.x=32768;body.right.z=56756;body.forward.x=-56756;body.forward.z=32768;
 for(int i=0;i<60;i++)ChaseCamera_Update(&record,&body,0);
 {float yaw=std::atan2(record.matrix.forward.x/65536.f,record.matrix.forward.z/65536.f);
  CHECK(std::abs(CameraOperator::Wrap(yaw-.5235988f))<.036f);
  float behind=record.matrix.position.x/65536.f*.5f+record.matrix.position.z/65536.f*.8660254f;
  CHECK(behind < -5.f);}
 CHECK(record.zoom>0xa000);
 car.steeringInput=65536;int tight=record.zoom;
 for(int i=0;i<80;i++)ChaseCamera_Update(&record,&body,0);
 CHECK(record.zoom<tight);car.steeringInput=0;
 body.right={0,0,65536};body.forward={-65536,0,0};
 // Same car/body/speed/turn, actual classic and drone Update/BuildMatrix output.
 car.speed=98304;body.position={0,0,0};body.right={0,0,65536};body.forward={-65536,0,0};
 FixVector savedOffsets[6],savedViews[6];memcpy(savedOffsets,g_chaseCameraOffsets,sizeof savedOffsets);memcpy(savedViews,g_chaseCameraViewOffsets,sizeof savedViews);
 extern void ChaseCamera_SetOffset(BYTE,FixVector*);FixVector configured={0,98304,425984};ChaseCamera_SetOffset(0,&configured);
 extern void ChaseCamera_SetPitch(BYTE,short);ChaseCamera_SetPitch(0,0x2d);
 Sys_SetOption("video.drone_distance",50);
 for(int variant=0;variant<2;variant++){
  g_droneVariant[0]=variant;CameraOperator_Reset(0);record.matrix={};
  g_chaseCameraOffsets[2]={0,65536,0};g_chaseCameraViewOffsets[2]={0,0,65536};
  for(int i=0;i<300;i++)ChaseCamera_Update(&record,&body,0);
  auto&m=record.matrix;
  printf("perspective %d %g %g %g %g %g %g %g\n",variant,m.position.x/65536.f,m.position.y/65536.f,m.position.z/65536.f,m.forward.x/65536.f,m.forward.y/65536.f,m.forward.z/65536.f,record.zoom/40960.f);
  if(variant==1){CHECK(m.position.z<-8.9f*65536);CHECK(m.position.y>2.1f*65536&&m.position.y<2.5f*65536);}   // tuned default height 60 lowers the hover: measured 2.20 (window was 2.2..2.5 at height 100)
 }
 // Actual updater under acceleration/cruise/braking: visibly independent,
 // but keeps the car and road in frame. Hold the exact same heading.
 for(int hz:{25,60,120}){
  g_physicsTimeStep=int(65536.f*25/hz);body.position={0,0,0};CameraOperator_Reset(0);g_droneVariant[0]=1;
  float carZ=0,maxLag=0,minLag=0;
  for(int i=0;i<hz*10;i++){
   float t=float(i)/hz,v=t<1?0:t<2?(t-1)*28:t<4?28:t<5?(5-t)*28:0;
   carZ+=v/hz;body.position.z=int(carZ*65536);ChaseCamera_Update(&record,&body,0);
   float trail=carZ-record.matrix.position.z/65536.f,lag=trail-9.f;
   maxLag=std::max(maxLag,lag);minLag=std::min(minLag,lag);
   CHECK(trail>6.5f&&trail<11.5f);CHECK(record.matrix.forward.z>59000);
  }
  CHECK(maxLag>1.f&&minLag<-1.f);CHECK(std::abs(carZ-record.matrix.position.z/65536.f-9.f)<.02f);
 }
 g_physicsTimeStep=65536;body.position={0,0,0};
 memcpy(g_chaseCameraOffsets,savedOffsets,sizeof savedOffsets);memcpy(g_chaseCameraViewOffsets,savedViews,sizeof savedViews);
 g_droneVariant[0]=1;
 Sys_SetOption("video.camera_operator",0);
 g_chaseCameraOffsets[2]={0,65536,0};g_chaseCameraViewOffsets[2]={65536,0,0};
 ChaseCamera_Update(&record,&body,0);
 CHECK(g_chaseCameraViewOffsets[2].x!=0); // original lag is restored immediately
 CHECK(g_chaseCameraLag[0]==0xe0000);
 // Optional corner roll: actual chase update, camera forward/position unchanged by roll.
 Sys_SetOption("video.camera_operator",0);Sys_SetOption("video.camera_roll",100);car.speed=98304;record.type=5;
 for(int i=0;i<30;i++){
  float angle=i*.025f;body.right.x=int(std::sin(angle)*65536);body.right.z=int(std::cos(angle)*65536);
  body.forward.x=-body.right.z;body.forward.z=body.right.x;
  ChaseCamera_Update(&record,&body,0);
 }
 printf("rollbasis %g %g %g %g %g %g %g %g %g\n",record.matrix.right.x/65536.f,record.matrix.right.y/65536.f,record.matrix.right.z/65536.f,record.matrix.up.x/65536.f,record.matrix.up.y/65536.f,record.matrix.up.z/65536.f,record.matrix.forward.x/65536.f,record.matrix.forward.y/65536.f,record.matrix.forward.z/65536.f);
 CHECK(std::abs(record.matrix.right.y)>100); // actual built basis leans, not just helper state
 Sys_SetOption("video.camera_roll",0);
 // Other camera modes keep original behavior even if the option is enabled.
 Sys_SetOption("video.camera_operator",1);g_playerCameraMode[0]=0;
 g_chaseCameraOffsets[2]={0,65536,0};g_chaseCameraViewOffsets[2]={65536,0,0};
 ChaseCamera_Update(&record,&body,0);CHECK(g_chaseCameraViewOffsets[2].x!=0);
 std::filesystem::current_path(old);std::filesystem::remove_all(tmp);
 puts("actual ChaseCamera_Update world-space camera and original spring restoration pass; native feel unverified");
 return 0;
}
