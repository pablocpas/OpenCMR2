#include "port/types.h"
#include "DustPlume.h"
#include <cstdio>
int main(){
    Particle p={};p.age=DUST_PLUME_LIFE<<16;p.position={0x2000,0x800,0};
    p.sourceVector={100<<16,0,200<<16};FixVector wind={0,0,0};int fail=0;
    int start=p.sourceVector.x,early=0,late=0,maxAlpha=0;
    FILE *out=fopen("dust-motion.csv","w");if(out)fputs("step,x,y,z,radius,alpha\n",out);
    for(int i=1;i<DUST_PLUME_LIFE;++i){p.age-=0x10000;DustPlume_Step(p,wind);
        if(i==20)early=p.sourceVector.x;if(i==140)late=p.sourceVector.x;
        if(p.type0x56>maxAlpha)maxAlpha=p.type0x56;
        if(p.type0x56>75 || p.size>0x36000 || p.size<0x6000)++fail;
        if(out)fprintf(out,"%d,%f,%f,%f,%f,%d\n",i,p.sourceVector.x/65536.,p.sourceVector.y/65536.,p.sourceVector.z/65536.,p.size/65536.,p.type0x56);
    }
    if(out)fclose(out);
    if(early<=start || late<=early || p.position.x>=0x2000 || p.type0x56>2 || maxAlpha<50 || p.sourceVector.y<=0)++fail;
    // Live cap 96 at the original 100%; budget guard respects it.
    if(DustPlume_LiveCap(100)!=96 || DustPlume_LiveCap(0)!=0 ||
       DustPlume_LiveCap(200)!=192 || DustPlume_LiveCap(300)!=288 ||
       DustPlume_LiveCap(999)!=288 || DustPlume_LiveCap(-5)!=0) ++fail;
    if(!DustPlume_HasBudget(335,95,400,96) || DustPlume_HasBudget(336,0,400,96) ||
       DustPlume_HasBudget(0,96,400,96) || DustPlume_HasBudget(0,0,0,96) ||
       DustPlume_HasBudget(0,0,64,96)) ++fail;
    // The 200% default raises the cap to 192; the aggregate reserve (64) stays.
    if(DustPlume_HasBudget(0,192,1200,192) || !DustPlume_HasBudget(0,191,1200,192) ||
       DustPlume_HasBudget(1136,0,1200,192) || !DustPlume_HasBudget(1135,0,1200,192)) ++fail;
    if(!DustPlume_Surface(true,false,false,false,false) ||
       !DustPlume_Surface(false,true,false,false,false) ||
       DustPlume_Surface(true,false,true,false,false) ||
       DustPlume_Surface(false,false,false,true,false) ||
       DustPlume_Surface(false,false,false,false,true) ||
       DustPlume_Surface(false,false,false,false,false)) ++fail;
    float r=.12f,g=.20f,b=.01f;DustPlume_SoilTint(r,g,b);
    if(r<=g || b!=.01f)++fail;
    r=.5f;g=1.f;b=.2f;DustPlume_SoilTint(r,g,b);if(r>1.f||g>=r)++fail;
    r=.6f;g=.4f;b=.2f;DustPlume_SoilTint(r,g,b);
    if(r!=.6f||g!=.4f||b!=.2f)++fail;
    r=.7f;g=.25f;b=.10f;DustPlume_CloudTint(r,g,b);
    if(!(r>g&&g>b&&r-b<.30f))++fail;
    r=.3f;g=.3f;b=.3f;DustPlume_CloudTint(r,g,b);
    if(std::fabs(r-.3f)>.00001f || std::fabs(g-.3f)>.00001f || std::fabs(b-.3f)>.00001f)++fail;
    puts(fail?"FAIL dust plume":"dust plume: bounded growth, drift decay, lift and fade pass");return fail?1:0;
}
