#include "port/types.h"
#include "render/effect_settings.h"
#include <map>
#include <string>
#include <cstdio>
static std::map<std::string,int> settings;static int blur,amount,bloom,grade,shadow,soft,particles,dof,rays,fog=-1,fogAmount=-1,fogGlow=-1,fogClear=-1;
extern "C" int Sys_GetOption(const char*k,int fallback){auto i=settings.find(k);return i==settings.end()?fallback:i->second;}
extern "C" void Gfx_SetMotionBlur(BOOL on,int mix){blur=on;amount=mix;}
extern "C" void Gfx_SetBloom(BOOL on,int){bloom=on;}
extern "C" void Gfx_SetParticles(int type,int){particles=type;}
extern "C" void Gfx_SetShadowQuality(int size,int softness){shadow=size;soft=softness;}
extern "C" void Gfx_SetShadows(int,int){}
extern "C" void Gfx_SetDepthOfField(BOOL on,int,int,int){dof=on;}
extern "C" void Gfx_SetGodRays(BOOL on,int){rays=on;}
extern "C" void Gfx_SetPostFog(BOOL on,int a,int g){fog=on;fogAmount=a;fogGlow=g;}
extern "C" void Gfx_SetPostFogClearDistance(int u){fogClear=u;}
extern "C" void Gfx_SetColorGrading(int temp,int,int sat,int contrast){grade=temp+sat+contrast-200;}
#define CHECK(x) do{if(!(x)){printf("FAIL line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 EffectSettings::Apply();CHECK(blur&&bloom&&particles==1&&amount==40&&shadow==1024&&soft==25&&grade==0&&!dof&&!rays);CHECK(settings.empty());
 CHECK(fog==0&&fogAmount==65&&fogGlow==60);   // fog defaults: off, 65 percent amount, modest glow
 CHECK(fogClear==0);   // near-field clear distance defaults off
 settings={{"video.motion_blur",0},{"video.bloom",0},{"video.color_grading",1},{"video.shadow_quality",2},{"video.shadow_softness",200},{"video.depth_of_field",1},{"video.god_rays",1},{"video.particles",2}};
 auto before=settings;EffectSettings::Apply();CHECK(!blur&&!bloom&&grade>0&&shadow==2048&&soft==100&&dof&&rays&&particles==2);CHECK(before==settings);
 settings["video.grade_amount"]=0;EffectSettings::Apply();CHECK(grade==0);
 settings["video.fog"]=1;settings["video.fog_amount"]=250;settings["video.fog_glow"]=-10;EffectSettings::Apply();
 CHECK(fog==1&&fogAmount==200&&fogGlow==0);   // fog amount clamps to 200, glow to 0..100
 settings["video.fog_amount"]=30;settings["video.fog_glow"]=80;EffectSettings::Apply();
 CHECK(fogAmount==30&&fogGlow==80);
 settings["video.fog_clear"]=250;EffectSettings::Apply();
 CHECK(fogClear==200);   // clear distance clamps to 0..200 world units
 puts("saved effect startup restore, nonneutral grade, fog bounds and no writes pass");
}
