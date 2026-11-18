#ifndef OPENCMR2_POSTPROCESS_PASS_H
#define OPENCMR2_POSTPROCESS_PASS_H
#include <SDL3/SDL.h>
// Colour-only passes must omit the depth descriptor entirely.
inline SDL_GPURenderPass *Gfx_BeginColourOnlyPass(SDL_GPUCommandBuffer *cmd,
    SDL_GPUTexture *target,SDL_GPULoadOp load) {
    if(!cmd || !target)return nullptr;
    SDL_GPUColorTargetInfo colour={};colour.texture=target;
    colour.load_op=load;colour.store_op=SDL_GPU_STOREOP_STORE;
    colour.clear_color={0,0,0,1};
    return SDL_BeginGPURenderPass(cmd,&colour,1,nullptr);
}
// Full-screen pipelines have no depth target. A nonnull descriptor with a
// null texture is NOT equivalent to no attachment (Metal dereferences it).
inline SDL_GPURenderPass *Gfx_BeginPostProcessPass(SDL_GPUCommandBuffer *cmd,
    SDL_GPUGraphicsPipeline *pipeline,SDL_GPUTexture *target,
    const SDL_GPUTextureSamplerBinding *bindings,int count) {
    if(!cmd || !pipeline || !target || !bindings || count<=0 || count>7)return nullptr;
    for(int i=0;i<count;++i)
        if(!bindings[i].texture || !bindings[i].sampler || bindings[i].texture==target)return nullptr;
    return Gfx_BeginColourOnlyPass(cmd,target,SDL_GPU_LOADOP_CLEAR);
}
// Two-target variant for the motion-blur blend (targetA = history slot, targetB = the back
// buffer or a scratch stand-in): both attachments must be distinct and no bound source may
// be either target. The full-screen triangle covers every pixel, so both load as CLEAR.
inline SDL_GPURenderPass *Gfx_BeginPostProcessPass2(SDL_GPUCommandBuffer *cmd,
    SDL_GPUGraphicsPipeline *pipeline,SDL_GPUTexture *targetA,SDL_GPUTexture *targetB,
    const SDL_GPUTextureSamplerBinding *bindings,int count) {
    if(!cmd || !pipeline || !targetA || !targetB || targetA==targetB || !bindings || count<=0 || count>7)return nullptr;
    for(int i=0;i<count;++i)
        if(!bindings[i].texture || !bindings[i].sampler || bindings[i].texture==targetA || bindings[i].texture==targetB)return nullptr;
    SDL_GPUColorTargetInfo colour[2]={};
    for(int i=0;i<2;++i){colour[i].texture=i?targetB:targetA;colour[i].load_op=SDL_GPU_LOADOP_CLEAR;colour[i].store_op=SDL_GPU_STOREOP_STORE;colour[i].clear_color={0,0,0,1};}
    return SDL_BeginGPURenderPass(cmd,colour,2,nullptr);
}
#endif
