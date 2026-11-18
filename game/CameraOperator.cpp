#include "CameraOperator.h"
#include "CameraRecord.h"
#include "Car.h"
#include "RallyRoute.h"
#include "port/sys.h"
static CameraOperator::State states[8];
bool Track_CameraGroundHeight(FixVector*,int*);
int RallyData_GetActiveCarRaceRecordField0(Car*);
int RallyData_GetRouteAdditionalState();
extern BYTE g_playerCameraMode[8];
extern BYTE g_droneVariant[8];
bool CameraOperator_Enabled(const CameraRecord*record){
 return record && record->index<8 && g_playerCameraMode[record->index]==4 && g_droneVariant[record->index] && Sys_GetOption("video.camera_operator",0);
}
void CameraOperator_Reset(unsigned i){if(i<8)states[i].Reset();}
void CameraOperator_Update(CameraRecord*record,const FixMatrix*reference,const FixVector*offset,short pitch){
 if(!record||record->index>=8)return;auto&state=states[record->index];
 if(!reference||!CameraOperator_Enabled(record)||record->car>=8){state.Reset();return;}
 Car*car=Car_Get(record->car);if(!car||!car->pWorld){state.Reset();return;}
 float x=reference->right.x/65536.f,z=reference->right.z/65536.f;
 if(!std::isfinite(x)||!std::isfinite(z)||x*x+z*z<.25f){state.Reset();return;}
 state.pitch=(pitch-0x2d)*(6.28318530718f/4096.f);
 auto&p=reference->position;
 CameraOperator::V3 world={p.x/65536.f,p.y/65536.f,p.z/65536.f};
 auto road=CameraOperator::PreviewRoad(world,RallyData_GetActiveCarRaceRecordField0(car),g_routeNodes?g_routeNodeCount:0,
  RallyData_GetRouteAdditionalState()!=0,[](int i){auto&p=g_routeNodes[i].position;return CameraOperator::V3{p.x/65536.f,0,p.z/65536.f};},
  1.f+.6f*CameraOperator::Clamp(car->speed/65536.f/1.5f,0.f,1.f)); // look further down the road at speed (12/30 world units at rest, up to 19.2/48)
 // Remove last terrain lift before advancing free world inertia.
 if(state.ready)state.position.y-=state.terrainLift;
 state.lensSpeedGain=0.f; // The drone lens no longer changes with speed (user request). video.drone_speed_lens is kept as an accepted but unused key; the steering lens stays.
 state.shakeAmount=0.f; // Speed shake belongs to the regular camera, not the drone (user request). video.drone_shake is kept as an accepted but unused key.
 // OpenCMR2: widened rails (user request, defaults unchanged): distance
 // 10..300, height 25..400, lateral 10..400, spring 1..400, drop 0..500.
 {int pct=Sys_GetOption("video.drone_distance",150);if(pct<10)pct=10;if(pct>300)pct=300;state.distanceScale=pct/100.f;}
 state.heightScale=CameraOperator::Clamp(Sys_GetOption("video.drone_height",60)/100.f,.25f,4.f);
 state.lateralGain=CameraOperator::Clamp(Sys_GetOption("video.drone_lateral",70)/100.f,.1f,4.f);
 state.springGain=CameraOperator::Clamp(Sys_GetOption("video.drone_spring",150)/100.f,.01f,4.f);
 state.speedDropAmount=CameraOperator::Clamp(Sys_GetOption("video.drone_speed_drop",150)/100.f,0.f,5.f);
 state.Step({p.x/65536.f,p.y/65536.f,p.z/65536.f},std::atan2(x,z),car->steeringInput/65536.f,car->speed/65536.f,
  offset?std::abs(offset->z/65536.f):6.5f,offset?offset->y/65536.f:1.5f,g_physicsTimeStep/(65536.f*25.f),&road);
 state.ConstrainTerrain({p.x/65536.f,p.y/65536.f,p.z/65536.f},g_physicsTimeStep/(65536.f*25.f),[](CameraOperator::V3 point,float&y){
  FixVector fixed={(int)std::lround(point.x*65536),(int)std::lround(point.y*65536),(int)std::lround(point.z*65536)};int height;
  if(!Track_CameraGroundHeight(&fixed,&height))return false;y=height/65536.f;return true;
 });
}
void CameraOperator_Apply(CameraRecord*record){
 if(!CameraOperator_Enabled(record)||!states[record->index].ready)return;
 const auto&s=states[record->index];auto&m=record->matrix;
 CameraOperator::V3 f=s.FramedForward();
 CameraOperator::V3 r=CameraOperator::Unit({f.z,0,-f.x});
 CameraOperator::V3 u={f.y*r.z-f.z*r.y,f.z*r.x-f.x*r.z,f.x*r.y-f.y*r.x};
 auto fixed=[](CameraOperator::V3 v){return FixVector{(int)std::lround(v.x*65536),(int)std::lround(v.y*65536),(int)std::lround(v.z*65536)};};
 // Keep the saved custom tilt adjustment relative to its original default.
 float c=std::cos(s.pitch),sn=std::sin(s.pitch);auto oldForward=f;
 f=f*c+u*sn;u=u*c-oldForward*sn;
 m.position=fixed(s.position+r*s.shakeRight+u*s.shakeUp);m.forward=fixed(f);m.right=fixed(r);m.up=fixed(u);
 record->zoom=std::lround(0xa000*s.zoom);
 record->lookDistance=std::lround((s.aim-s.position).Length()*65536);
 record->field_0x50=record->lookDistance-0x10000;
}
