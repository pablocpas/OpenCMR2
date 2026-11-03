#ifndef OPENCMR2_EFFECT_AMOUNTS_H
#define OPENCMR2_EFFECT_AMOUNTS_H
#include <cstdint>
inline int Effect_Clamp(int value){return value<0?0:value>100?100:value;}
inline int Effect_Scale(int value,int percent){return int(int64_t(value)*Effect_Clamp(percent)/100);}
// Carry fractional accumulation so low strengths still accumulate gradually.
inline int Effect_Accumulate(int amount,int percent,int &remainder){
 int64_t total=int64_t(amount)*Effect_Clamp(percent)+remainder;
 remainder=int(total%100);return int(total/100);
}
struct EffectAmountRow{const char*label;const char*key;int initial,count;};
inline const EffectAmountRow* EffectAmount_Rows(int &count){
 static const EffectAmountRow rows[]={
 {"Motion blur %","video.motion_blur_amount",60,101},
 {"Bloom %","video.bloom_amount",24,101},
 {"Precip density %","video.precipitation_amount",47,101},
 {"Shadow softness %","video.shadow_softness",25,101},
 {"Shadow map quality","video.shadow_quality",1,3},
 {"Defocus blur %","video.dof_blur",100,101},
 {"Sun rays %","video.god_rays_amount",39,101},
 {"Grade blend %","video.grade_amount",100,101},
 {"Camera shake %","video.camera_shake_amount",100,101},
 {"Lens dirt","video.lens_dirt",1,2},
 {"Lens dirt %","video.lens_dirt_amount",100,101},
 {"Sun glare % (0 off)","video.sun_glare",60,101},
 {"Lens flare","video.lens_flare",1,2},
 {"Lens flare %","video.lens_flare_amount",100,101},
 {"Dust plume","video.dust_plume",1,2},
 {"Dust amount %","video.dust_amount",100,101},
 {"Body paint dirt","video.body_dirt",1,2},
 {"World dust","video.dust_volume",1,2},
 {"Clouds (disabled)","video.volumetric_clouds",0,2},
 {"Cube reflection size","video.reflection_resolution",128,4},
 };
 count=sizeof(rows)/sizeof(*rows);return rows;
}
#endif
