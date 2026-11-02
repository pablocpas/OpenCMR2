#ifndef OPENCMR2_EFFECT_SETTINGS_H
#define OPENCMR2_EFFECT_SETTINGS_H
#include "port/sys.h"
#include "port/gfx.h"
namespace EffectSettings {
inline int Clamp(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
inline int Percent(const char* key,int fallback){return Clamp(Sys_GetOption(key,fallback),0,100);}
inline int Byte(const char* key,int fallback){return Percent(key,fallback)*255/100;}
// Shared by both executors at startup and menu Apply. No writes.
inline void Apply(){
 Gfx_SetMotionBlur(Sys_GetOption("video.motion_blur",1),Byte("video.motion_blur_amount",16));
 Gfx_SetBloom(Sys_GetOption("video.bloom",1),Byte("video.bloom_amount",24));
 Gfx_SetParticles(Clamp(Sys_GetOption("video.particles",1),0,2),Byte("video.precipitation_amount",47));
 Gfx_SetShadowQuality(Clamp(Sys_GetOption("video.shadow_quality",1),0,2)==0?512:Sys_GetOption("video.shadow_quality",1)==2?2048:1024,Percent("video.shadow_softness",25));
 Gfx_SetShadows(Sys_GetOption("video.shadows",1),5);
 Gfx_SetDepthOfField(Sys_GetOption("video.depth_of_field",0),Percent("video.dof_focus",50),Percent("video.dof_range",50),Percent("video.dof_blur",30));
 Gfx_SetGodRays(Sys_GetOption("video.god_rays",0),Byte("video.god_rays_amount",39));
 Gfx_SetPostFog(Sys_GetOption("video.fog",0),Clamp(Sys_GetOption("video.fog_amount",65),0,200),Percent("video.fog_glow",60));
 Gfx_SetPostFogClearDistance(Clamp(Sys_GetOption("video.fog_clear",0),0,200));
 int strength=Sys_GetOption("video.color_grading",0)?Percent("video.grade_amount",100):0;
 // Restrained filmic grade. At zero this is mathematically neutral.
 Gfx_SetColorGrading(8*strength/100,-2*strength/100,100+6*strength/100,100+5*strength/100);
}
}
#endif
