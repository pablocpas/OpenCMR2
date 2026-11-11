#include "port/types.h"
#include "DustPlume.h"
#include "Graphics.h"
#include "Texture.h"
#include <cstring>
#include "port/gfx.h"
#include <cstdio>
extern int g_billboardsEnabled,g_billboardsRequested,g_unk0x006dd784;
extern int g_billboardRuns[800][2];
extern void Particle_DrawAll(SceneNode*,BYTE);
extern void Particle_ClearActiveFlags();
extern Particle *g_particles;extern ParticleType *g_particleTypes;
extern int g_particleCount,g_particleTypeCount,g_nextParticle;
static int volumeCalls;static bool available=true;static GfxDustLobe last;
static int volumeEnabled=1,plumeAmount=200;
extern "C" int __wrap_Sys_GetOption(const char *key,int fallback){return !std::strcmp(key,"video.dust_volume")?volumeEnabled:!std::strcmp(key,"video.dust_plume_amount")?plumeAmount:fallback;}
extern "C" BOOL __wrap_Gfx_DrawDustVolume(const GfxDustLobe *lobes,int count){volumeCalls++;if(count)last=lobes[0];return available&&count>0&&lobes[0].centreRadius[0]==10;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){Particle pool[400]={};ParticleType types[33]={};g_particles=pool;g_particleTypes=types;g_particleCount=400;g_particleTypeCount=33;g_nextParticle=0;
 auto &type=types[DUST_PLUME_TYPE];type.flags=1;type.lifetime=DUST_PLUME_LIFE<<16;Texture smoke={};smoke.textureId=1;type.texture=&smoke;
 FixVector point={10<<16,0,20<<16},velocity={0,0,0};BYTE colour[]={120,90,50,255};
 DustPlume_Emit(&point,&velocity,colour,0,1);DustPlume_AdvanceWorld(65536);
 for(auto &p:pool)if(p.active){p.size=65536;p.type0x53=80;p.texture=type.texture;}
 g_billboardsEnabled=0;g_billboardsRequested=0;
 Particle_DrawAll(nullptr,0);CHECK(volumeCalls==1&&g_billboardsRequested==0);
 // An original dry wheel sheet is replaced only when explicitly tagged.
 Particle extra={};extra.active=1;extra.field0x55=1;extra.pType=&types[0x12];
 Texture leaf={};leaf.textureId=7;extra.texture=&leaf;extra.size=65536;extra.type0x53=80;extra.dryWheelDust=true;
 pool[399]=extra;Particle_DrawAll(nullptr,0);CHECK(volumeCalls==2&&g_billboardsRequested==0);
 pool[399].dryWheelDust=false;Particle_DrawAll(nullptr,0);CHECK(volumeCalls==3&&g_billboardsRequested==1);
 pool[399].active=0;g_billboardsRequested=0;
 // Count real queue entries, not the disabled-queue boolean request latch.
 // Unsupported/disabled volume must still be cloud-only: no dry leaf sheets.
 g_billboardsEnabled=1;g_unk0x006dd784=0;
 pool[399]=extra;available=false;Particle_DrawAll(nullptr,0);
 CHECK(volumeCalls==4&&g_unk0x006dd784==1);
 pool[399].dryWheelDust=false;g_unk0x006dd784=0;Particle_DrawAll(nullptr,0);
 CHECK(volumeCalls==5&&g_unk0x006dd784==2);
 pool[399].dryWheelDust=true;g_unk0x006dd784=0;volumeEnabled=0;
 Particle_DrawAll(nullptr,0);CHECK(volumeCalls==5&&g_unk0x006dd784==1);
 pool[399].active=0;
 Particle_ClearActiveFlags();available=true;volumeEnabled=1;
 Particle_DrawAll(nullptr,0);CHECK(volumeCalls==5);
 // Real integration copies per-step wheel ejection into volume m/s and soil tint.
 velocity={-0x4000,0,0};BYTE olive[]={32,52,3,255};
 DustPlume_Emit(&point,&velocity,olive,0,1);
 BYTE fallbackTint[3]={};bool foundCloud=false;
 for(auto &p:pool)if(p.active&&p.pType==&type){
  for(int c=0;c<3;c++)fallbackTint[c]=p.colour[c];foundCloud=true;
 }
 CHECK(foundCloud);
 DustPlume_AdvanceWorld(25*65536);
 Particle_DrawAll(nullptr,0);
 CHECK(last.centreRadius[0]<7&&last.centreRadius[0]>5);
 CHECK(last.centreRadius[2]==20&&last.colourDensity[0]>last.colourDensity[1]);
 for(int c=0;c<3;c++)CHECK(std::fabs(last.colourDensity[c]-fallbackTint[c]/255.f)<.001f);
 // Cloud smoke is desaturated, not the bright leaf/grass colour it replaced.
 CHECK(fallbackTint[0]>fallbackTint[1]&&fallbackTint[1]>fallbackTint[2]);
 CHECK(fallbackTint[0]-fallbackTint[2]<30);
 // Repeated draw selection cannot mutate the particle owner, life or tag.
 pool[399]=extra;Particle snapshot=pool[399];Particle_DrawAll(nullptr,0);
 CHECK(!std::memcmp(&snapshot,&pool[399],sizeof snapshot));
 Particle_ClearActiveFlags();
 // Zero cloud cap must retain a soft-smoke wheel fallback, not leaf art.
 volumeEnabled=0;plumeAmount=0;
 DustPlume_Emit(&point,&velocity,colour,0,1);
 int liveClouds=0;for(auto &p:pool)if(p.active&&p.pType==&type)liveClouds++;
 CHECK(liveClouds==0);
 pool[399]=extra;g_unk0x006dd784=0;
 Particle_DrawAll(nullptr,0);CHECK(g_unk0x006dd784==1);CHECK(g_billboardRuns[0][1]==smoke.textureId);
 Particle_ClearActiveFlags();
 // Pool reserve exhausted by non-cloud particles: baseline dry fallback too.
 plumeAmount=200;
 for(int i=0;i<336;i++){pool[i]={};pool[i].active=1;pool[i].pType=&types[0];}
 DustPlume_Emit(&point,&velocity,colour,0,1);
 liveClouds=0;for(auto &p:pool)if(p.active&&p.pType==&type)liveClouds++;
 CHECK(liveClouds==0);pool[399]=extra;g_unk0x006dd784=0;
 Particle_DrawAll(nullptr,0);CHECK(g_unk0x006dd784==1);CHECK(g_billboardRuns[0][1]==smoke.textureId);
 Particle_ClearActiveFlags();
 // Recycling an explicitly tagged particle through the original API clears the tag.
 types[0x12].flags=1;types[0x12].lifetime=65536;g_nextParticle=0;
 Particle_SpawnTagged(0x12,&point,&velocity,0,nullptr,colour,0,nullptr,1,true);
 CHECK(pool[0].dryWheelDust);pool[0].active=0;g_nextParticle=0;
 Particle_Spawn(0x12,&point,&velocity,0,nullptr,colour,0,nullptr,1);
 CHECK(!pool[0].dryWheelDust);
 g_particles=nullptr;g_particleTypes=nullptr;g_particleCount=g_particleTypeCount=0;puts("cloud-only dry dirt; volume/fallback tint, non-dry preservation and stage clear: pass");}
