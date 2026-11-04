#ifndef OPENCMR2_FOG_UNIFORMS_H
#define OPENCMR2_FOG_UNIFORMS_H
// The ONE packer for the FogUniforms std140 block (shaders/fog.frag). Both
// executors (SDL_GPU in gfx_gpu.cpp, GL in gl_effects_executor.h) call this:
// the 23-vs-27-vec4 stack overflow came from two hand-copied fill loops whose
// size constant drifted from the shader when beams[8] was added. Layout:
// fog, sun, extra, lights[8], lightInfo[8], beams[8], halos[8], camA, camB = 37 vec4 = 148 floats
// (kFogUniformFloats). Every float is written explicitly - no zero-init
// reliance - so the pushed block never carries stale or unwritten lanes.
#include "port/gfx.h"
#include "render/fog_math.h"
#include "render/fog_camera.h"
#include <cstdio>
#include <cstdlib>

struct FogUniformParams {
    float density;        // Fog_Density(amount)
    float glow;           // fog_glow / 100
    float nearZ, farZ;    // recovered projection, 0 = unknown (GL path)
    float sunX, sunY;     // projected sun uv (v downwards)
    float sunVisible;     // 0/1
    float aspect;         // backWidth / backHeight
    float clearDistance;  // world units, 0 = off
    float lightStrength;  // video.fog_lights 0..2
    float depthHalfRange; // 1 = GL half-range depth buffer (un-remap d=2*depth-1 in the shader); 0 = full-range (SDL_GPU)
};

// Both executors pack each light's depthRef for the depth gate; the GL
// executor remaps it into its half-range depth buffer in SetFogLights.
inline void FogUniforms_Pack(float *out, const FogUniformParams &p,
                             const GfxFogLight *lights, int lightCount) {
    out[0] = p.density;        out[1] = p.glow;        out[2] = p.nearZ;      out[3] = p.farZ;
    if (getenv("FOG_DEBUG")) {
        fprintf(stderr, "FOGDBG nearZ=%.4f farZ=%.4f aspect=%.3f strength=%.2f\n", p.nearZ, p.farZ, p.aspect, p.lightStrength);
        for (int i = 0; i < lightCount && i < 8; i++)
            fprintf(stderr, "FOGDBG L%d ref=%.4f endRef=%.4f q=%.3f len=%.3f\n", i, lights[i].depthRef, lights[i].beamEndRef, lights[i].beamFade, lights[i].beamLen);
    }
    out[4] = p.sunX;           out[5] = p.sunY;        out[6] = p.sunVisible; out[7] = p.aspect;
    out[8] = p.clearDistance;  out[9] = p.lightStrength; out[10] = p.depthHalfRange; out[11] = 0.0f;
    for (int i = 0; i < 8; i++) {
        const bool on = i < lightCount && lights != nullptr;
        const GfxFogLight *L = on ? &lights[i] : nullptr;
        out[12 + i*4 + 0] = on ? L->x : 0.0f;
        out[12 + i*4 + 1] = on ? L->y : 0.0f;
        out[12 + i*4 + 2] = on ? L->intensity : 0.0f;
        out[12 + i*4 + 3] = on ? L->startRamp : 0.0f;   // lights[i].w: wedge start ramp (0 = none)
        out[44 + i*4 + 0] = on ? L->depthRef : 0.0f;
        out[44 + i*4 + 1] = on ? L->r : 0.0f;
        out[44 + i*4 + 2] = on ? L->g : 0.0f;
        out[44 + i*4 + 3] = on ? L->b : 0.0f;
        out[76 + i*4 + 0] = on ? L->beamX : 0.0f;
        out[76 + i*4 + 1] = on ? L->beamY : 0.0f;
        out[76 + i*4 + 2] = on ? L->beamLen : 0.0f;
        out[76 + i*4 + 3] = on ? L->beamFade : 1.0f;
        out[108 + i*4 + 0] = on ? L->haloX : 0.0f;
        out[108 + i*4 + 1] = on ? L->haloY : 0.0f;
        out[108 + i*4 + 2] = on ? L->haloRef : 0.0f;
        out[108 + i*4 + 3] = on ? L->beamEndRef : 0.0f;   // beam-end depth ref for the segment gate
    }
    // World-anchored banks (render/fog_camera.h): camA = pos xz, forward xz; camB = right xz, 1/p11, strength.
    out[140] = g_fogCamera.posX;   out[141] = g_fogCamera.posZ;   out[142] = g_fogCamera.fwdX;  out[143] = g_fogCamera.fwdZ;
    out[144] = g_fogCamera.rightX; out[145] = g_fogCamera.rightZ; out[146] = g_fogCamera.invP11; out[147] = g_fogCamera.strength;
}
#endif
