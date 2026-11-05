#ifndef OPENCMR2_COCKPIT_EXTENSION_H
#define OPENCMR2_COCKPIT_EXTENSION_H
#include "port/gfx.h"
#include <vector>
#include <cmath>
struct CockpitExtensionProfile {
    float top[3], bottom[3], floorInner[3];
    float uv[4][2]; // source patch: top rear/forward, bottom rear/forward
    float repeatLength;
};
// Emit connected wall + floor to a common rear plane. Each bounded cell
// reuses the actual source UV patch, never a single sampled colour, atlas
// wrap, or a synthetic mirrored gradient. Partial last cells crop the patch.
inline void CockpitExtension_Append(std::vector<GfxVertex>&out,
                                    const CockpitExtensionProfile&p,float rear)
{
    float start=std::fmax(p.top[0],std::fmax(p.bottom[0],p.floorInner[0]));
    float length=start-rear;
    if(length<=0 || p.repeatLength<=.01f)return;
    int cells=(int)std::ceil(length/p.repeatLength);if(cells>128)cells=128;
    auto lerp=[](float a,float b,float t){return a+(b-a)*t;};
    for(int c=0;c<cells;c++){
        float a=std::fmin(c*p.repeatLength/length,1.f),b=std::fmin((c+1)*p.repeatLength/length,1.f);
        float fraction=(b-a)*length/p.repeatLength;
        float q[6][3];
        const float*src[3]={p.top,p.bottom,p.floorInner};
        for(int k=0;k<3;k++)for(int xyz=0;xyz<3;xyz++){
            q[k*2][xyz]=xyz==0?lerp(src[k][0],rear,a):src[k][xyz];
            q[k*2+1][xyz]=xyz==0?lerp(src[k][0],rear,b):src[k][xyz];
        }
        const int ids[12]={0,1,3,0,3,2,2,3,5,2,5,4};
        for(int j=0;j<12;j++){
            int k=ids[j];GfxVertex v={};v.x=q[k][0];v.y=q[k][1];v.z=q[k][2];
            v.ny=j>=6?1:0;v.nz=j<6?-1:0;v.diffuse=0xffc0c0c0;
            // Along the strip, restart the original rear->forward UV range.
            bool bottom=k>=2;int r=bottom?2:0;float t=k%2?fraction:0;
            v.u0=lerp(p.uv[r][0],p.uv[r+1][0],t);
            v.v0=lerp(p.uv[r][1],p.uv[r+1][1],t);out.push_back(v);
        }
    }
}
#endif
