#include "render/postprocess_pass.h"
#include <cstdio>
static int calls=0,fail=0;
static SDL_GPULoadOp expectedLoad=SDL_GPU_LOADOP_CLEAR;
static Uint32 expectedCount=1;
extern "C" SDL_GPURenderPass *SDL_BeginGPURenderPass(SDL_GPUCommandBuffer*,const SDL_GPUColorTargetInfo *c,Uint32 n,const SDL_GPUDepthStencilTargetInfo *d){
 ++calls;if(d || n!=expectedCount || !c)++fail;
 else for(Uint32 i=0;i<n;++i)if(!c[i].texture || c[i].load_op!=expectedLoad || c[i].store_op!=SDL_GPU_STOREOP_STORE)++fail;
 return reinterpret_cast<SDL_GPURenderPass*>(0x10);
}
int main(){
 auto cmd=reinterpret_cast<SDL_GPUCommandBuffer*>(1);auto pipe=reinterpret_cast<SDL_GPUGraphicsPipeline*>(2);
 auto target=reinterpret_cast<SDL_GPUTexture*>(3);auto source=reinterpret_cast<SDL_GPUTexture*>(4);auto sampler=reinterpret_cast<SDL_GPUSampler*>(5);
 SDL_GPUTextureSamplerBinding b={source,sampler};
 if(!Gfx_BeginPostProcessPass(cmd,pipe,target,&b,1)||calls!=1)++fail;
 if(Gfx_BeginPostProcessPass(cmd,nullptr,target,&b,1) || Gfx_BeginPostProcessPass(cmd,pipe,nullptr,&b,1) ||
    Gfx_BeginPostProcessPass(nullptr,pipe,target,&b,1) || Gfx_BeginPostProcessPass(cmd,pipe,target,nullptr,1) ||
    Gfx_BeginPostProcessPass(cmd,pipe,target,&b,0))++fail;
 b.texture=target;if(Gfx_BeginPostProcessPass(cmd,pipe,target,&b,1))++fail;
 b.texture=nullptr;if(Gfx_BeginPostProcessPass(cmd,pipe,target,&b,1))++fail;
 b.texture=source;b.sampler=nullptr;if(Gfx_BeginPostProcessPass(cmd,pipe,target,&b,1))++fail;
 if(calls!=1)++fail;
 expectedLoad=SDL_GPU_LOADOP_LOAD;
 if(!Gfx_BeginColourOnlyPass(cmd,target,SDL_GPU_LOADOP_LOAD)||calls!=2)++fail;
 if(Gfx_BeginColourOnlyPass(cmd,nullptr,SDL_GPU_LOADOP_LOAD)||calls!=2)++fail;
 // Two-target variant (the motion-blur blend mirrors into the back buffer): two distinct
 // attachments, no bound source texture may be either target, null targets rejected.
 expectedLoad=SDL_GPU_LOADOP_CLEAR;expectedCount=2;
 auto targetB=reinterpret_cast<SDL_GPUTexture*>(6);
 b.texture=source;b.sampler=sampler;
 if(!Gfx_BeginPostProcessPass2(cmd,pipe,target,targetB,&b,1)||calls!=3)++fail;
 if(Gfx_BeginPostProcessPass2(cmd,pipe,target,nullptr,&b,1)||Gfx_BeginPostProcessPass2(cmd,pipe,nullptr,targetB,&b,1)||
    Gfx_BeginPostProcessPass2(cmd,pipe,target,target,&b,1)||Gfx_BeginPostProcessPass2(nullptr,pipe,target,targetB,&b,1)||
    Gfx_BeginPostProcessPass2(cmd,nullptr,target,targetB,&b,1))++fail;
 b.texture=targetB;if(Gfx_BeginPostProcessPass2(cmd,pipe,target,targetB,&b,1))++fail;
 b.texture=target;if(Gfx_BeginPostProcessPass2(cmd,pipe,target,targetB,&b,1))++fail;
 b.texture=source;
 if(calls!=3)++fail;
 printf("postprocess pass: %s, no dummy depth, failed resources rejected\n",fail?"FAIL":"pass");return fail?1:0;
}
