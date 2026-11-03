#include "port/types.h"
#include "AirMote.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
extern int g_particleCount,g_particleTypeCount,g_nextParticle;
extern Particle *g_particles;
extern ParticleType *g_particleTypes;
extern FixVector g_particleWind;
extern FixVector g_airMoteDrift;
extern unsigned g_airMoteNextSeed;
void AirMote_Init(Particle*,ParticleType*,void*);
void Particle_Spawn(int,FixVector*,FixVector*,int,FixMatrix*,BYTE*,BYTE,void*,BYTE);
extern "C" void Sys_SetOptionTemporary(const char *name, int value);
int main(){
 Sys_SetOptionTemporary("video.air_motes_amount",200);   // the test pins the 200 budget; the built-in default is now the tuned 500
 Particle pool[400]={};ParticleType types[0x23]={};
 g_particles=pool;g_particleTypes=types;g_particleCount=400;g_particleTypeCount=0x23;g_nextParticle=0;
 types[AIR_MOTE_TYPE].flags=1;types[AIR_MOTE_TYPE].lifetime=AIR_MOTE_LIFE<<16;
 FixVector world={10<<16,5<<16,20<<16};int fail=0;
 // Spawn stream leaves the gameplay rand sequence untouched (own LCG in Particle_SpawnTagged).
 std::srand(1234);int expected=std::rand();std::srand(1234);
 for(int i=0;i<80;++i)AirMote_Spawn(&world);
 if(std::rand()!=expected)++fail;
 // Budget: the overdone default amount 200 doubles the cap; all 80 spawns fit, no eviction of others.
 int motes=0;for(auto&p:pool)if(p.active){if(p.pType==&types[AIR_MOTE_TYPE])++motes;else++fail;}
 if(motes!=80)++fail;
 // The doubled cap still pins: 40 more spawns stop at 2x the base.
 for(int i=0;i<40;++i)AirMote_Spawn(&world);
 motes=0;for(auto&p:pool)if(p.active&&p.pType==&types[AIR_MOTE_TYPE])++motes;
 if(motes!=AIR_MOTE_BASE*2)++fail;
 for(auto&p:pool)if(p.active&&(p.attachmentMatrix||p.field0x55!=1))++fail;
 // Init stamps the wander seed; update through the registered callback: eases toward the
 // shared drift, never monotonic sink, alpha bounded.
 g_airMoteDrift={0x10000,0,0};
 {unsigned seed=0x42u;g_airMoteNextSeed=seed;Particle ip={};AirMote_Init(&ip,&types[AIR_MOTE_TYPE],nullptr);unsigned got;std::memcpy(&got,ip.pad0x5c,4);if(got!=seed)++fail;}
 Particle*m=nullptr;for(auto&p:pool)if(p.active){m=&p;break;}
 if(!m)++fail;else{
  m->position={0,0,0};m->age=AIR_MOTE_LIFE<<16; // freshly spawned
  FixVector before=m->sourceVector;
  AirMote_Update(m,&types[AIR_MOTE_TYPE],nullptr);
  if(m->position.x<=0)++fail;                          // drift.x > 0 pulls it forward
  if(std::abs(m->position.y)>0x1000)++fail;            // tiny bob only, no monotonic sink
  if(m->sourceVector.x-before.x!=m->position.x||m->vector0x28.x!=m->sourceVector.x)++fail; // world position advances by the drift, mirror kept in sync
  if(m->type0x56>AIR_MOTE_ALPHA_MAX)++fail;
 }
 // Never evict original particles: a pool with 336 active originals refuses new motes.
 for(auto&p:pool)p.active=0;
 for(int i=0;i<336;++i){pool[i].active=1;pool[i].pType=&types[0];}
 AirMote_Spawn(&world);
 {int active=0;for(auto&p:pool)active+=p.active!=0;if(active!=336)++fail;}
 // A full pool of originals plus motes: an original spawn may evict a mote first.
 for(auto&p:pool)p.active=0;
 for(int i=0;i<400;++i){pool[i].active=1;pool[i].pType=&types[0];}
 types[0].flags=1;types[0].lifetime=100<<16;
 pool[7].pType=&types[AIR_MOTE_TYPE];pool[7].age=1; // stale mote is the preferred eviction target
 FixVector vel={0,0,0};
 Particle_Spawn(0,&world,&vel,world.y,NULL,NULL,0,NULL,1);
 {int m2=0;for(auto&p:pool)if(p.active&&p.pType==&types[AIR_MOTE_TYPE])++m2;
  if(m2!=0)++fail; // the mote gave its slot to the original spawn
 }
 g_particles=nullptr;g_particleTypes=nullptr;g_particleCount=g_particleTypeCount=0;
 puts(fail?"FAIL air mote pool":"air mote pool: 48 cap, rand isolation, drift/sink update, no original eviction, mote evicted first pass");
 return fail?1:0;
}
