#include "port/types.h"
#include "DustPlume.h"
#include <cstdio>
#include <cstdlib>
extern int g_particleCount,g_particleTypeCount,g_nextParticle;
extern Particle *g_particles;
extern ParticleType *g_particleTypes;
extern "C" void Sys_SetOptionTemporary(const char *name, int value);
int main(){
 Sys_SetOptionTemporary("video.dust_plume_amount",200);   // the test pins the 200 cap maths; the built-in default is now the tuned 300
 Particle pool[400]={};ParticleType types[33]={};
 g_particles=pool;g_particleTypes=types;g_particleCount=400;g_particleTypeCount=33;g_nextParticle=0;
 types[DUST_PLUME_TYPE].flags=1;types[DUST_PLUME_TYPE].lifetime=DUST_PLUME_LIFE<<16;
 FixVector world={10<<16,0,20<<16},v={0x1000,0x800,0};BYTE c[4]={100,90,70,255};
 std::srand(1234);int expected=std::rand();std::srand(1234);
 int fail=0;for(int i=0;i<140;++i)DustPlume_Emit(&world,&v,c,0,1);
 if(std::rand()!=expected)++fail;
 int active=0;for(auto &p:pool)if(p.active){++active;if(p.attachmentMatrix || p.sourceVector.x!=world.x || p.field0x55!=1)++fail;}
 if(active!=140)++fail;   // default amount 200: live cap 192, 140 emits all fit
 for(int i=0;i<120;++i)DustPlume_Emit(&world,&v,c,0,1);
 active=0;for(auto &p:pool)active+=p.active!=0;if(active!=192)++fail;   // cap pins at LiveCap(200)
 for(auto &p:pool)p.active=0;
 for(int i=0;i<336;++i){pool[i].active=1;pool[i].pType=&types[0];}
 DustPlume_Emit(&world,&v,c,0,1);
 active=0;for(auto &p:pool)active+=p.active!=0;if(active!=336)++fail;
 g_particleCount=0;DustPlume_Emit(&world,&v,c,0,1);
 puts(fail?"FAIL cloud pool":"cloud pool:192cap at default 200,64reservedslots,world-space and disabledpool pass");
 g_particles=nullptr;g_particleTypes=nullptr;g_particleCount=g_particleTypeCount=0;return fail?1:0;
}
