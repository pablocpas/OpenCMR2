#include "CornerCameraRoll.h"
#include "CameraRecord.h"
#include "Car.h"
#include "port/sys.h"
static CornerCameraRoll::State states[8];
extern int g_physicsTimeStep;
void CornerCameraRoll_Reset(unsigned i){if(i<8)states[i].Reset();}
void CornerCameraRoll_Update(CameraRecord*r,const FixMatrix*ref){
 if(!r||r->index>=8)return;
 auto&s=states[r->index];int amount=Sys_GetOption("video.camera_roll",100);
 Car*c=r->car<8?Car_Get(r->car):nullptr;
 if(!ref||!c||r->type<4||r->type>6){s.Reset();return;}
 s.Step(std::atan2(ref->right.x/65536.f,ref->right.z/65536.f),c->speed/65536.f,g_physicsTimeStep/(65536.f*25.f),amount);
}
void CornerCameraRoll_Apply(CameraRecord*r){
 if(!r||r->index>=8||r->type<4||r->type>6||Sys_GetOption("video.camera_roll",100)<=0)return;
 auto&s=states[r->index];if(!s.ready||s.angle==0)return;
 auto right=r->matrix.right,up=r->matrix.up;float c=std::cos(s.angle),sn=std::sin(s.angle);
 auto roll=[&](int a,int b){return int(std::lround(a*c+b*sn));};
 r->matrix.right={roll(right.x,up.x),roll(right.y,up.y),roll(right.z,up.z)};
 r->matrix.up={roll(up.x,-right.x),roll(up.y,-right.y),roll(up.z,-right.z)};
}
