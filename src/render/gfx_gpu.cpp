#include "sun_glare.h"
#include "render/dof_gate.h"
#include "render/fog_math.h"
#include "render/fog_uniforms.h"
#include "render/tunnel_vision.h"
#include "render/dof_focus.h"
#include "render/car_region.h"
#include "render/vapor_trail.h"
#include "render/speed_streak.h"
#include "render/weather_glint.h"
#include "render/projection_vote.h"
#include "shadow_caster.h"
#include "cloud_uniforms.h"
#include "temporal_uniforms.h"
// The SDL_GPU executor: runs a recorded frame (gfx_core.cpp) with one set of
// shaders that emulate Direct3D 7's fixed-function pipeline.

#include "platform/platform.h"
#include "port/sys.h"
#include "render/presentation.h"
#include "render/gpu_startup_options.h"
#include "render/dust_volume_math.h"
#include "render/postprocess_pass.h"
#include "render/gfx_internal.h"

#include "shaders_spirv.h"

// SPIR-V and, when built, MSL code of one shader for Executor::Shader.
#ifdef OPENCMR2_SHADERS_MSL
#define SHADER_CODE(name) g_##name, sizeof(g_##name), g_##name##_msl, sizeof(g_##name##_msl)
#else
#define SHADER_CODE(name) g_##name, sizeof(g_##name), nullptr, 0
#endif

#include <math.h>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <unordered_map>

namespace {

// ---- uniform blocks (std140, see shaders/common.glsl and fixed.frag) ----

struct LightUniform {
    float diffuse[4], specular[4], ambient[4], position[4], direction[4], attenuation[4], spot[4];
};

struct VertexUniforms {
    float worldView[16];
    float world[16];          // model-to-world, for shadow mapping
    float lightViewProj[16];  // light's view * projection, for shadow mapping
    float projection[16];
    float textureMatrix[2][16];
    float materialDiffuse[4], materialAmbient[4], materialSpecular[4], materialEmissive[4];
    float globalAmbient[4];
    LightUniform lights[8];
    int32_t lightFlags[4];
    int32_t sources[4];
    int32_t fogFlags[4];
    float fogParams[4];
    int32_t texCoordIndex[4];
    float target[4];
};

struct FragmentUniforms {
    int32_t stage0[4], stage0b[4], stage1[4], stage1b[4];
    float textureFactor[4];
    int32_t alphaTest[4];
    float alphaRef[4];
    float fogColor[4];
    int32_t fogMode[4];
    float fogParams[4];
    float bumpMatrix[4];
    float bumpLuminance[4];
    float distanceFade[4];
    float shadowParams[4];  // x: bias, y: enabled, -, -
    float bodyDirt[4];
};

struct ClearUniforms {
    float color[4];
    float depth[4];
};

void Multiply(const GfxMatrix &a, const GfxMatrix &b, float *out)
{
    const float *pa = &a._11, *pb = &b._11;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            float s = 0;
            for (int k = 0; k < 4; k++)
                s += pa[r * 4 + k] * pb[k * 4 + c];
            out[r * 4 + c] = s;
        }
}

void TransformPoint(const GfxVector &v, float w, const GfxMatrix &m, float out[3])
{
    const float *p = &m._11;
    for (int c = 0; c < 3; c++)
        out[c] = v.x * p[c] + v.y * p[4 + c] + v.z * p[8 + c] + w * p[12 + c];
}

void Color(DWORD c, float out[4])
{
    out[0] = ((c >> 16) & 0xff) / 255.0f;
    out[1] = ((c >> 8) & 0xff) / 255.0f;
    out[2] = (c & 0xff) / 255.0f;
    out[3] = ((c >> 24) & 0xff) / 255.0f;
}

void Copy4(const GfxColorValue &c, float out[4])
{
    out[0] = c.r;
    out[1] = c.g;
    out[2] = c.b;
    out[3] = c.a;
}

SDL_GPUBlendFactor BlendFactor(int d3d)
{
    switch (d3d) {
    case GFX_BLEND_ZERO: return SDL_GPU_BLENDFACTOR_ZERO;
    case GFX_BLEND_ONE: return SDL_GPU_BLENDFACTOR_ONE;
    case GFX_BLEND_SRCCOLOR: return SDL_GPU_BLENDFACTOR_SRC_COLOR;
    case GFX_BLEND_INVSRCCOLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
    case GFX_BLEND_SRCALPHA: return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case GFX_BLEND_INVSRCALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case GFX_BLEND_DESTALPHA: return SDL_GPU_BLENDFACTOR_DST_ALPHA;
    case GFX_BLEND_INVDESTALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
    case GFX_BLEND_DESTCOLOR: return SDL_GPU_BLENDFACTOR_DST_COLOR;
    case GFX_BLEND_INVDESTCOLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
    case GFX_BLEND_SRCALPHASAT: return SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE;
    default: return SDL_GPU_BLENDFACTOR_ONE;
    }
}

SDL_GPUCompareOp CompareOp(int d3d)
{
    switch (d3d) {
    case GFX_CMP_NEVER: return SDL_GPU_COMPAREOP_NEVER;
    case GFX_CMP_LESS: return SDL_GPU_COMPAREOP_LESS;
    case GFX_CMP_EQUAL: return SDL_GPU_COMPAREOP_EQUAL;
    case GFX_CMP_LESSEQUAL: return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    case GFX_CMP_GREATER: return SDL_GPU_COMPAREOP_GREATER;
    case GFX_CMP_NOTEQUAL: return SDL_GPU_COMPAREOP_NOT_EQUAL;
    case GFX_CMP_GREATEREQUAL: return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
    default: return SDL_GPU_COMPAREOP_ALWAYS;
    }
}

SDL_GPUPrimitiveType PrimitiveType(int d3d)
{
    switch (d3d) {
    case GFX_POINTLIST: return SDL_GPU_PRIMITIVETYPE_POINTLIST;
    case GFX_LINELIST: return SDL_GPU_PRIMITIVETYPE_LINELIST;
    case GFX_LINESTRIP: return SDL_GPU_PRIMITIVETYPE_LINESTRIP;
    case GFX_TRIANGLESTRIP: return SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
    default: return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    }
}

struct GpuTexture {
    SDL_GPUTexture *texture = nullptr;
    SDL_GPUTexture *depth = nullptr;       // render targets
    SDL_GPUTextureFormat format;
    bool cpuDecode = false;                // BC data decoded on the CPU (no BC support)
};

// The GPU copy of a GfxVertexBuffer, as of a version of it.
struct GpuVertexBuffer {
    SDL_GPUBuffer *buffer = nullptr;
    uint32_t capacity = 0;
    uint32_t version = 0;
};

// The device the renderer asks SDL_GPU for: its shader formats, and no
// optional feature it does not use.
SDL_PropertiesID DeviceProperties()
{
    SDL_GPUShaderFormat formats = SDL_GPU_SHADERFORMAT_SPIRV;
#ifdef OPENCMR2_SHADERS_MSL
    formats |= SDL_GPU_SHADERFORMAT_MSL;
#endif
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN,
                           (formats & SDL_GPU_SHADERFORMAT_SPIRV) != 0);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN,
                           (formats & SDL_GPU_SHADERFORMAT_MSL) != 0);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN,
                           Platform_GetSettingInt("video.gpu_debug", 0) != 0);
    // SDL asks Vulkan for these features by default. The renderer does not
    // use the first three, and many Android GPUs lack some, for which SDL
    // would refuse the device. Anisotropy is a required Vulkan feature and
    // the renderer now uses it by default (video.anisotropy), so it stays on.
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN, true);
    return props;
}

struct Executor : GfxExecutor {
    SDL_GPUDevice *device = nullptr;
    SDL_Window *window = nullptr;
    SDL_GPUShader *vs3d = nullptr, *vsTL = nullptr, *fs = nullptr, *vsClear = nullptr, *fsClear = nullptr;
    SDL_GPUShader *vsMotionBlur = nullptr, *fsMotionBlur = nullptr;
    SDL_GPUShader *fsBrightPass = nullptr, *fsBlur = nullptr, *fsComposite = nullptr;
    SDL_GPUShader *vsParticle = nullptr, *fsParticle = nullptr;
    SDL_GPUShader *vsShadow = nullptr, *fsShadow = nullptr;
    SDL_GPUShader *fsLensGrime = nullptr, *fsSpeedStreak = nullptr;
    SDL_GPUShader *fsDoF = nullptr, *fsGodRays = nullptr, *fsColorGrading = nullptr, *fsFog = nullptr;
    // OpenCMR2: optional single-frame AA, with an independent non-aliasing target.
    SDL_GPUShader *fsClouds=nullptr;
    SDL_GPUGraphicsPipeline *cloudPipeline=nullptr;
    SDL_GPUTexture *cloudTarget=nullptr;
    bool cloudLogged=false;
    // OpenCMR2: opt-in temporal AA (partial stage: camera reprojection only).
    SDL_GPUShader *fsTemporal=nullptr;
    SDL_GPUGraphicsPipeline *temporalPipeline=nullptr;
    SDL_GPUTexture *taaTex[2]={nullptr,nullptr};
    SDL_GPUTexture *taaPrevDepth=nullptr; // depth of the previous world pass, for disocclusion
    int taaIndex=0;
    TemporalTracker temporalTracker;
    bool temporalLogged=false;
    bool taaPrevDepthValid=false;
    SDL_GPUShader *fsSpatialAA = nullptr;
    SDL_GPUGraphicsPipeline *spatialAAPipeline = nullptr;
    SDL_GPUTexture *spatialAATarget = nullptr;
    SDL_GPUShader *fsDustVolume = nullptr, *fsDustComposite = nullptr;
    SDL_GPUGraphicsPipeline *dustVolumePipeline = nullptr, *dustCompositePipeline = nullptr;
    SDL_GPUTexture *dustVolumeTarget = nullptr, *dustCompositeTarget = nullptr;
    bool dustVolumeLogged=false;
    bool SupportsDustVolume() override {
        if(!sampledDepth || !backDepth || !backColor || !vsMotionBlur)return false;
        return PrepareDustResources();
    }
    bool spatialAA = false;
    bool metalFXSpatial = false;
    bool metalFXLogged = false;
    using MetalFXBlit = bool (*)(SDL_GPUCommandBuffer *, const SDL_GPUBlitInfo *);
    MetalFXBlit metalFXBlit = nullptr;
    SDL_GPUTexture *backColor = nullptr, *backDepth = nullptr;
    SDL_GPUTexture *backdrop = nullptr;    // the copy of backColor the distance fade reads
    SDL_GPUTexture *motionBlurPrevious = nullptr;  // the previous frame, for motion blur
    SDL_GPUTexture *motionBlurTemp = nullptr;      // the blended frame, before the swapchain blit
    SDL_GPUTexture *bloomBuf1 = nullptr, *bloomBuf2 = nullptr;  // bloom ping-pong buffers
    SDL_GPUGraphicsPipeline *motionBlurPipeline = nullptr;  // the full-screen blend pass
    SDL_GPUGraphicsPipeline *bloomBrightPassPipeline = nullptr;  // bright-pass extract
    SDL_GPUGraphicsPipeline *bloomBlurPipeline = nullptr;  // directional blur pass
    SDL_GPUGraphicsPipeline *bloomCompositePipeline = nullptr;  // scene + bloom additive
    SDL_GPUGraphicsPipeline *particlePipeline = nullptr;  // particle billboard pass
    SDL_GPUTexture *shadowMap = nullptr;   // directional light shadow map (color: depth in R channel)
    SDL_GPUTexture *shadowDepth = nullptr; // depth buffer for the shadow pass
    SDL_GPUGraphicsPipeline *shadowPipeline = nullptr;  // depth-only shadow pass
    int backWidth = 0, backHeight = 0;
    SDL_GPUTextureFormat colorFormat = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    SDL_GPUTextureFormat depthFormat = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
    SDL_GPUBuffer *vertexBuffer = nullptr, *indexBuffer = nullptr;
    uint32_t vertexCapacity = 0, indexCapacity = 0;
    SDL_GPUTransferBuffer *uploadBuffer = nullptr;  // the frame's vertices and indices
    uint32_t uploadCapacity = 0;
    SDL_GPUTexture *white2D = nullptr, *whiteCube = nullptr;
    std::unordered_map<uint32_t, SDL_GPUGraphicsPipeline *> pipelines;
    std::unordered_map<uint32_t, SDL_GPUSampler *> samplers;
    bool bcSupported = true;
    bool presentModeVsync = true;
    // OpenCMR2: video.anisotropy defaults to 8 (the original sampled without
    // anisotropy; 1 restores it). video.smooth_scaling as before.
    int anisotropy = 8, smoothScaling = 1;
    // OpenCMR2: video.motion_blur — mix (0..255) of the previous frame into
    // the current one at present time; 0 turns it off.
    bool motionBlurEnabled = false;
    int motionBlurMix = 0;
    bool motionBlurHasPrevious = false;
    // OpenCMR2: video.bloom — enabled and intensity (0..255).
    bool bloomEnabled = false;
    int bloomIntensity = 0;
    // OpenCMR2: particles (rain/snow). type 0=off, 1=rain, 2=snow; intensity
    // 0..255 scales the emission rate.
    int particleType = 0;
    int particleIntensity = 0;
    uint32_t particleLastTicks = 0;
    // OpenCMR2: lens grime (chromatic aberration, vignette, film grain).
    float chromaticAberration = 0;
    float vignette = 0;
    float filmGrain = 0;
    SDL_GPUGraphicsPipeline *lensGrimePipeline = nullptr, *speedStreakPipeline = nullptr;
    SDL_GPUGraphicsPipeline *dofPipeline = nullptr;       // depth of field
    SDL_GPUGraphicsPipeline *godRaysPipeline = nullptr;   // volumetric light shafts
    SDL_GPUGraphicsPipeline *fogPipeline = nullptr;         // atmosphere fog (video.fog)
    SDL_GPUGraphicsPipeline *colorGradingPipeline = nullptr; // analytic color grading
    SDL_GPUTexture *gradingTarget = nullptr;
    bool sampledDepth = false;
    // Debug object mask: while maskDraw, only draws whose objectId == maskObject are re-issued (magenta, no blend, no depth write,
    // depth test LESS_EQUAL against the scene depth), so occluded parts of the car stay unpainted.
    bool maskDraw = false;
    unsigned maskObject = 0;
    static uint32_t MaskKey(uint32_t key)
    {
        key &= ~(1u << 4);           // no blend
        key &= ~(1u << 14);          // no depth write
        key |= 1u << 13;             // depth test on
        return (key & ~(15u << 15)) | ((uint32_t)GFX_CMP_LESSEQUAL & 15u) << 15;
    }
    // Projections of the draws that wrote the sampled depth (back buffer, untransformed, depth test+write),
    // recovered from _33/_43 and counted per distinct near/far pair. The dominant pair feeds the DoF pass.
    struct ProjVote { float n, f; int count; };
    ProjVote projVotes[4] = {}; int projVoteTotal = 0, projVoteRejected = 0;
    float sceneNear = 0.0f, sceneFar = 0.0f;
    void VoteProjection(const GfxMatrix &m)
    {
        float n, f;
        if (!Projection_NearFar(m._33, m._34, m._43, &n, &f)) { projVoteRejected++; return; }
        projVoteTotal++;
        for (ProjVote &v : projVotes)
            if (v.count && std::fabs(v.n - n) < 1e-3f * n && std::fabs(v.f - f) < 1e-3f * f) { v.count++; return; }
        for (ProjVote &v : projVotes)
            if (!v.count) { v = { n, f, 1 }; return; }
    }
    int lastResolveDistinct = 0, lastResolveVotes = 0; float lastResolveNear = 0.0f, lastResolveFar = 0.0f;   // diagnostic copy for the F10 fog line
    void ResolveProjection(float *outNear, float *outFar)
    {
        *outNear = *outFar = 0.0f;
        lastResolveDistinct = 0; lastResolveVotes = 0; lastResolveNear = lastResolveFar = 0.0f;
        int best = -1, distinct = 0;
        for (int i = 0; i < 4; ++i) { if (projVotes[i].count) distinct++; if (best < 0 || projVotes[i].count > projVotes[best].count) best = i; }
        if (best >= 0 && projVotes[best].count) {
            // More than one near/far pair among depth-writing draws: the whole-frame depth is ambiguous, so
            // keep the old raw-depth rule (near = far = 0 tells dof.frag to use it) instead of guessing.
            if (distinct == 1) { *outNear = projVotes[best].n; *outFar = projVotes[best].f; }
            lastResolveDistinct = distinct; lastResolveVotes = projVotes[best].count;
            lastResolveNear = projVotes[best].n; lastResolveFar = projVotes[best].f;
            static int lastDistinct = -1;
            if (distinct != lastDistinct) {
                lastDistinct = distinct;
                Sys_Log("options: DoF projection from depth-writing draws: %d distinct near/far pair(s), dominant near %.3f far %.1f (%d of %d draws); %s", distinct, projVotes[best].n, projVotes[best].f, projVotes[best].count, projVoteTotal, distinct == 1 ? "linear depth candidate in use" : "ambiguous: old raw-depth rule used");
            }
        }
        else {
            // No depth-writing draw produced a usable perspective projection: report it once (state change), never silently.
            static bool loggedNone = false;
            if (!loggedNone) {
                loggedNone = true;
                Sys_Log("options: DoF projection: 0 usable depth-writing draws this frame (%d rejected as non-perspective); old raw-depth rule used", projVoteRejected);
            }
        }
    }
    SDL_GPUTexture *dofBuf1 = nullptr;  // depth-aware disk blur output
    SDL_GPUTexture *godRaysBuf1 = nullptr; // normalized light-shaft output
    SDL_GPUTexture *fogBuf1 = nullptr;       // atmosphere fog output
    // OpenCMR2: depth of field state.
    bool dofEnabled = false;
    int dofFocalDistance = 50;   // 0..100, focus plane depth
    int dofFocalRange = 50;      // 0..100, in-focus band width
    int dofMaxBlur = 30;         // 0..100, max blur radius
    GfxDofStatus dofStatus = {0, 0, 0, 0, 0};
    bool speedEffectsSuppressed=false;
    float speedFocusTarget = 0.0f, speedFocusSmooth = 0.0f; // 0..1 tunnel vision; smoothed per frame
    Uint64 speedFocusTicks = 0;
    float speedStreakTarget = 0.0f, speedStreakSmooth = 0.0f, speedStreakClock = 0.0f; Uint64 speedStreakTicks = 0; // 0..1, smoothed in real time
    float vaporTarget = 0.0f, vaporSmooth = 0.0f; Uint64 vaporTicks = 0; VaporHistory vaporHistory; float vaporPrevCentre[3] = {}, vaporDir[3] = {}; bool vaporHavePrev = false, vaporHaveDir = false;
    // OpenCMR2: god rays state.
    SunGlareState sunGlare; Uint64 sunGlareTicks=0;
    bool godRaysEnabled = false;
    int godRaysIntensity = 100;
    // OpenCMR2: atmosphere fog state (video.fog). Amount is a menu percent (0..200 via CLI).
    bool fogEnabled = false;
    int fogAmount = 65, fogGlow = 60, fogClearDistance = 0;
    float sunX=.5f,sunY=.5f;bool sunVisible=false;
    int shadowSize=1024;float shadowSoftness=1.f;
    // OpenCMR2: color grading state.
    float colorGradingTemperature = 0.0f; // -1..1 cool->warm
    float colorGradingTint = 0.0f;        // -1..1 green->magenta
    float colorGradingSaturation = 100.0f;  // 0..2 color strength
    float colorGradingContrast = 100.0f;    // 0..2 contrast around middle gray
    // OpenCMR2: directional shadows. type 0=off, 1=on; bias in depth units.
    int shadowType = 0;
    float shadowBias = 0.0005f;
    // The sun's view and projection matrices, rebuilt when the stage changes.
    GfxMatrix lightView;
    GfxMatrix lightProj;
    float lightViewProj[16];  // lightProj * lightView, for shadow mapping
    // Particle pool: simulated and rendered entirely in the port layer.
    struct Particle {
        float x, y;          // normalised screen position (y=0 top)
        float vx, vy;        // screen-space velocity per second
        float sx, sy;        // half-size in screen fraction
        float r, g, b, a;    // colour
        float life;          // seconds remaining
        bool active;
    };
    static const int kMaxParticles = 2000;
    Particle particles[kMaxParticles];
    SDL_GPUBuffer *particleVB = nullptr;
    uint32_t particleVBCapacity = 0;
    uint32_t particleCount = 0;
    Uint32 presentWidth = 0, presentHeight = 0; // the window's pixels, as last logged
    // Back buffer reads of recently presented frames (the lens flare test).
    // The game gets the newest one the GPU has finished, a frame or two old:
    // waiting for the last frame instead stopped the CPU from working ahead
    // of the GPU, which halved the frame rate on phones with the sun in view.
    struct Readback {
        SDL_GPUTransferBuffer *buffer = nullptr;
        uint32_t capacity = 0;
        SDL_GPUFence *fence = nullptr;
        uint64_t serial = 0;
        int width = 0, height = 0;
    };
    Readback readbacks[3];
    uint64_t readbackSerial = 0;

    ~Executor() override
    {
        if (device == NULL)
            return;
        SDL_WaitForGPUIdle(device);
        for (Readback &r : readbacks) {
            SDL_ReleaseGPUFence(device, r.fence);
            SDL_ReleaseGPUTransferBuffer(device, r.buffer);
        }
        for (auto &p : pipelines)
            SDL_ReleaseGPUGraphicsPipeline(device, p.second);
        for (auto &s : samplers)
            SDL_ReleaseGPUSampler(device, s.second);
        SDL_ReleaseGPUTexture(device, dustCompositeTarget);
        dustCompositeTarget = nullptr;
        SDL_ReleaseGPUTexture(device, dustVolumeTarget);
        dustVolumeTarget = nullptr;
        SDL_ReleaseGPUTexture(device,cloudTarget);cloudTarget=nullptr;
        for(auto &t:taaTex){SDL_ReleaseGPUTexture(device,t);t=nullptr;}SDL_ReleaseGPUTexture(device,taaPrevDepth);taaPrevDepth=nullptr;taaPrevDepthValid=false;temporalTracker.Reset();
        SDL_ReleaseGPUTexture(device, spatialAATarget);
        SDL_ReleaseGPUTexture(device, backColor);
        SDL_ReleaseGPUTexture(device, backDepth);
        SDL_ReleaseGPUTexture(device, backdrop);
        SDL_ReleaseGPUTexture(device, motionBlurPrevious);
        SDL_ReleaseGPUTexture(device, motionBlurTemp);
        SDL_ReleaseGPUTexture(device, bloomBuf1);
        SDL_ReleaseGPUTexture(device, bloomBuf2);
        SDL_ReleaseGPUTexture(device, gradingTarget);
        SDL_ReleaseGPUTexture(device, dofBuf1);
        SDL_ReleaseGPUTexture(device, godRaysBuf1);
        SDL_ReleaseGPUTexture(device, fogBuf1);
        SDL_ReleaseGPUTexture(device, white2D);
        SDL_ReleaseGPUTexture(device, whiteCube);
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_ReleaseGPUTransferBuffer(device, uploadBuffer);
        SDL_ReleaseGPUShader(device, vs3d);
        SDL_ReleaseGPUShader(device, vsTL);
        SDL_ReleaseGPUShader(device, fs);
        SDL_ReleaseGPUShader(device, vsClear);
        SDL_ReleaseGPUShader(device, fsClear);
        SDL_ReleaseGPUShader(device, vsMotionBlur);
        SDL_ReleaseGPUShader(device, fsMotionBlur);
        SDL_ReleaseGPUShader(device, fsBrightPass);
        SDL_ReleaseGPUShader(device, fsBlur);
        SDL_ReleaseGPUShader(device, fsComposite);
        SDL_ReleaseGPUShader(device, vsParticle);
        SDL_ReleaseGPUShader(device, fsParticle);
        SDL_ReleaseGPUShader(device, fsLensGrime);
        SDL_ReleaseGPUShader(device, fsSpeedStreak);
        SDL_ReleaseGPUShader(device, fsDoF);
        SDL_ReleaseGPUShader(device, fsFog);
        SDL_ReleaseGPUShader(device,fsClouds);
        SDL_ReleaseGPUShader(device,fsTemporal);
        SDL_ReleaseGPUGraphicsPipeline(device,temporalPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device,cloudPipeline);
        SDL_ReleaseGPUShader(device, fsGodRays);
        SDL_ReleaseGPUShader(device, fsSunGlare);
        SDL_ReleaseGPUGraphicsPipeline(device, sunGlarePipeline);
        SDL_ReleaseGPUShader(device, fsColorGrading);
        SDL_ReleaseGPUShader(device, fsDustComposite);
        SDL_ReleaseGPUGraphicsPipeline(device, dustCompositePipeline);
        SDL_ReleaseGPUShader(device, fsDustVolume);
        SDL_ReleaseGPUGraphicsPipeline(device, dustVolumePipeline);
        SDL_ReleaseGPUShader(device, fsSpatialAA);
        SDL_ReleaseGPUGraphicsPipeline(device, spatialAAPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, motionBlurPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, bloomBrightPassPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, bloomBlurPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, bloomCompositePipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, particlePipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, shadowPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, lensGrimePipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, speedStreakPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, dofPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, godRaysPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, fogPipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, colorGradingPipeline);
        SDL_ReleaseGPUBuffer(device, particleVB);
        SDL_ReleaseGPUTexture(device, shadowMap);
        SDL_ReleaseGPUTexture(device, shadowDepth);
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
    }

    // The shader format the device takes: SPIR-V (Vulkan) or MSL (Metal).
    SDL_GPUShaderFormat shaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;

    SDL_GPUShader *Shader(const unsigned char *spirv, size_t spirvSize, const unsigned char *msl, size_t mslSize,
                          SDL_GPUShaderStage stage, int samplers, int uniforms)
    {
        SDL_GPUShaderCreateInfo info = {};
        if (shaderFormat == SDL_GPU_SHADERFORMAT_MSL) {
            info.code = msl;
            info.code_size = mslSize - 1; // without the NUL the embedding adds
            info.entrypoint = "main0";    // SPIRV-Cross renames main for MSL
        } else {
            info.code = spirv;
            info.code_size = spirvSize;
            info.entrypoint = "main";
        }
        info.format = shaderFormat;
        info.stage = stage;
        info.num_samplers = samplers;
        info.num_uniform_buffers = uniforms;
        SDL_GPUShader *shader = SDL_CreateGPUShader(device, &info);
        if (shader == NULL)
            Sys_Log("gfx: shader: %s", SDL_GetError());
        return shader;
    }

    SDL_GPUTexture *CreateTexture(SDL_GPUTextureType type, SDL_GPUTextureFormat format, int w, int h, int layers,
                                  int levels, SDL_GPUTextureUsageFlags usage)
    {
        SDL_GPUTextureCreateInfo info = {};
        info.type = type;
        info.format = format;
        info.usage = usage;
        info.width = w;
        info.height = h;
        info.layer_count_or_depth = layers;
        info.num_levels = levels;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        SDL_GPUTexture *t = SDL_CreateGPUTexture(device, &info);
        if (t == NULL)
            Sys_Log("gfx: texture %dx%d: %s", w, h, SDL_GetError());
        return t;
    }

    bool CreateDevice()
    {
        window = Platform_GetWindow();
        if (window == NULL) {
            Sys_Log("gfx: no window");
            return false;
        }
        anisotropy = Platform_GetSettingInt("video.anisotropy", 8);
        smoothScaling = Platform_GetSettingInt("video.smooth_scaling", 1);
        const auto startupOptions=GPUStartupOptions::Read();
        spatialAA=startupOptions.spatialAA;
        metalFXSpatial=startupOptions.metalFXSpatial;
#ifdef OPENCMR2_SHADERS_MSL
        Sys_Log("options: compiled GPU shader formats=SPIR-V,MSL");
#else
        Sys_Log("options: compiled GPU shader formats=SPIR-V only (no Metal MSL)");
#endif
        SDL_PropertiesID props = DeviceProperties();
        device = SDL_CreateGPUDeviceWithProperties(props);
        SDL_DestroyProperties(props);
        if (device == NULL) {
            Sys_Log("gfx: SDL_CreateGPUDevice: %s", SDL_GetError());
            return false;
        }
        if (!(SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV))
            shaderFormat = SDL_GPU_SHADERFORMAT_MSL;
        if (!SDL_ClaimWindowForGPUDevice(device, window)) {
            Sys_Log("gfx: SDL_ClaimWindowForGPUDevice: %s", SDL_GetError());
            return false;
        }
        Sys_Log("gfx: SDL_GPU driver %s", SDL_GetGPUDeviceDriver(device));
        Sys_Log("options: GPU shader format=%s; spatial_aa requested=%d; MetalFX spatial requested=%d",
            shaderFormat==SDL_GPU_SHADERFORMAT_MSL ? "MSL" : "SPIR-V",spatialAA,metalFXSpatial);
        // Function pointer supplied only by our pinned SDL Metal backend extension.
        // No application code accesses native handles or casts opaque SDL structs.
        metalFXBlit = reinterpret_cast<MetalFXBlit>(SDL_GetPointerProperty(
            SDL_GetGPUDeviceProperties(device), "OpenCMR2.MetalFXSpatialBlit", nullptr));
        Sys_Log("options: MetalFX extension=%s (available is not proof of an encoded frame)",metalFXBlit ? "available" : "unavailable");
        if (metalFXSpatial && metalFXBlit == nullptr)
            Sys_Log("gfx: MetalFX extension unavailable; using ordinary scaling");
        if (!SDL_GPUTextureSupportsFormat(device, depthFormat, SDL_GPU_TEXTURETYPE_2D,
                                          SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
            depthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        // Metal cannot sample D24S8. Prefer sampleable D32 for depth effects,
        // and retain ordinary depth rendering if the device cannot provide it.
        sampledDepth = SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
            SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
        Sys_Log("options: sampled depth support=%d (depth effects still require successful passes)",sampledDepth);
        if (sampledDepth)
            depthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        bcSupported = SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM,
                                                   SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_SAMPLER);

        vs3d = Shader(SHADER_CODE(fixed3d_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        vsTL = Shader(SHADER_CODE(fixedtl_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        fs = Shader(SHADER_CODE(fixed_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 8, 1);
        vsClear = Shader(SHADER_CODE(clear_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        fsClear = Shader(SHADER_CODE(clear_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);
        vsMotionBlur = Shader(SHADER_CODE(motionblur_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
        fsMotionBlur = Shader(SHADER_CODE(motionblur_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 1);
        fsBrightPass = Shader(SHADER_CODE(brightpass_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
        fsBlur = Shader(SHADER_CODE(blur_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
        fsComposite = Shader(SHADER_CODE(composite_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 1);
        fsLensGrime = Shader(SHADER_CODE(lens_grime_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
        fsSpeedStreak = Shader(SHADER_CODE(speedstreak_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 1);
        fsDoF = Shader(SHADER_CODE(dof_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 1);
        fsFog = Shader(SHADER_CODE(fog_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 1);
        fsGodRays = Shader(SHADER_CODE(godRays_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 3, 1);
        fsColorGrading = Shader(SHADER_CODE(colorGrading_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
        vsParticle = Shader(SHADER_CODE(particle_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        fsParticle = Shader(SHADER_CODE(particle_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
        vsShadow = Shader(SHADER_CODE(shadow_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        fsShadow = Shader(SHADER_CODE(shadow_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
        if (!vs3d || !vsTL || !fs || !vsClear || !fsClear || !vsMotionBlur || !fsMotionBlur ||
            !fsBrightPass || !fsBlur || !fsComposite || !fsLensGrime || !fsSpeedStreak || !fsDoF || !fsGodRays || !fsFog ||
            !fsColorGrading || !vsParticle || !fsParticle || !vsShadow || !fsShadow)
            return false;

        white2D = CreateTexture(SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM, 1, 1, 1, 1,
                                SDL_GPU_TEXTUREUSAGE_SAMPLER);
        whiteCube = CreateTexture(SDL_GPU_TEXTURETYPE_CUBE, SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM, 1, 1, 6, 1,
                                  SDL_GPU_TEXTUREUSAGE_SAMPLER);
        uint32_t white[6] = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff };
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, sizeof(white), 0 };
        SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
        memcpy(SDL_MapGPUTransferBuffer(device, tb, false), white, sizeof(white));
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        for (int layer = 0; layer < 7; layer++) {
            SDL_GPUTextureTransferInfo src = { tb, (Uint32)(layer < 6 ? layer * 4 : 0), 0, 0 };
            SDL_GPUTextureRegion dst = {};
            dst.texture = layer < 6 ? whiteCube : white2D;
            dst.layer = layer < 6 ? layer : 0;
            dst.w = dst.h = dst.d = 1;
            SDL_UploadToGPUTexture(copy, &src, &dst, false);
        }
        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(device, tb);

        // The pipelines the game draws with (keys of PipelineKey: menus,
        // stages, replays, results), made now: the first use of each could
        // take a frame's time to compile when the driver has not cached it.
        static const uint32_t common[] = {
            0x001a0459, 0x001a045b, 0x001a04b9, 0x001a0cb8, 0x001a0cb9, 0x001a0cbb, 0x001a2458, 0x001a2cb8,
            0x001a64b8, 0x001a6ca8, 0x001a6cb4, 0x001a6cb8, 0x000a2458, 0x000a2cb8, 0x000a6ca8, 0x000a6cb8,
        };
        for (uint32_t key : common)
            Pipeline(key);
        for (int clear = 1; clear < 4; clear++)
            ClearPipeline((clear & 1) != 0, (clear & 2) != 0);
        return true;
    }

    bool SetVideoMode(int width, int height, bool fullscreen) override
    {
        if (device == NULL && !CreateDevice())
            return false;
        if (backColor == NULL || backWidth != width || backHeight != height) {
            SDL_WaitForGPUIdle(device);
            SDL_ReleaseGPUTexture(device, dustCompositeTarget);
            dustCompositeTarget = nullptr;
            SDL_ReleaseGPUTexture(device, dustVolumeTarget);
            dustVolumeTarget = nullptr;
            SDL_ReleaseGPUTexture(device,cloudTarget);cloudTarget=nullptr;
            for(auto &t:taaTex){SDL_ReleaseGPUTexture(device,t);t=nullptr;}SDL_ReleaseGPUTexture(device,taaPrevDepth);taaPrevDepth=nullptr;taaPrevDepthValid=false;temporalTracker.Reset();
        SDL_ReleaseGPUTexture(device, spatialAATarget);
            spatialAATarget = nullptr;
            SDL_ReleaseGPUTexture(device, backColor);
            SDL_ReleaseGPUTexture(device, backDepth);
            SDL_ReleaseGPUTexture(device, backdrop);
            SDL_ReleaseGPUTexture(device, motionBlurPrevious);
            SDL_ReleaseGPUTexture(device, motionBlurTemp);
            SDL_ReleaseGPUTexture(device, bloomBuf1);
            SDL_ReleaseGPUTexture(device, bloomBuf2);
            SDL_ReleaseGPUTexture(device, dofBuf1);
            SDL_ReleaseGPUTexture(device, godRaysBuf1);
            SDL_ReleaseGPUTexture(device, fogBuf1);
            SDL_ReleaseGPUTexture(device, gradingTarget);
            SDL_ReleaseGPUTexture(device, shadowMap);
            SDL_ReleaseGPUTexture(device, shadowDepth);
            shadowMap = NULL;
            shadowDepth = NULL;
            backColor = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                      SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            backDepth = CreateTexture(SDL_GPU_TEXTURETYPE_2D, depthFormat, width, height, 1, 1,
                SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET |
                (sampledDepth ? SDL_GPU_TEXTUREUSAGE_SAMPLER : 0));
            backdrop = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                     SDL_GPU_TEXTUREUSAGE_SAMPLER);
            motionBlurPrevious = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                               SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            motionBlurTemp = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                           SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            bloomBuf1 = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                      SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            bloomBuf2 = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                      SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            gradingTarget = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            dofBuf1 = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                    SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            godRaysBuf1 = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            fogBuf1 = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                    SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            // Shadow map is created lazily when shadows are first enabled,
            // because D24_UNORM_S8_UINT + sampler is not valid on Metal.
            shadowMap = NULL;
            backWidth = width;
            backHeight = height;
            if (backColor == NULL || backDepth == NULL || backdrop == NULL ||
                motionBlurPrevious == NULL || motionBlurTemp == NULL ||
                bloomBuf1 == NULL || bloomBuf2 == NULL ||
                dofBuf1 == NULL || godRaysBuf1 == NULL || fogBuf1 == NULL || gradingTarget == NULL)
                return false;
        }
        SDL_SetWindowResizable(window, true);
        SetFullscreen(fullscreen);
        SDL_ShowWindow(window);
        BuildLightMatrices();
        return true;
    }

    void SetFullscreen(bool fullscreen) override
    {
        Gfx_SetWindowMode(window, fullscreen, backWidth, backHeight);
    }

    // Builds the sun's view and projection matrices for shadow mapping: a
    // directional light from the upper right, looking at the origin, with an
    // orthographic projection covering the stage.
    void BuildLightMatrices()
    {
        // Sun direction (where the light comes from): upper right, slightly
        // behind the camera.
        const float dir[3] = { 0.35f, -0.8f, -0.55f };
        float len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
        float lx = dir[0] / len, ly = dir[1] / len, lz = dir[2] / len;
        // Light looks at the origin from -dir * dist.
        const float dist = 200.0f;
        float ex = -lx * dist, ey = -ly * dist, ez = -lz * dist;
        float fx = lx, fy = ly, fz = lz; // looking toward origin
        float ux = 0.0f, uy = 1.0f, uz = 0.0f;
        if (fabsf(fy) > 0.99f) { ux = 0.0f; uy = 0.0f; uz = 1.0f; }
        // s = normalize(cross(f, up)), u = cross(s, f)
        float sx = fy * uz - fz * uy, sy = fz * ux - fx * uz, sz = fx * uy - fy * ux;
        float sl = sqrtf(sx * sx + sy * sy + sz * sz);
        sx /= sl; sy /= sl; sz /= sl;
        float u2x = sy * fz - sz * fy, u2y = sz * fx - sx * fz, u2z = sx * fy - sy * fx;
        // Row-major D3D view matrix.
        memset(&lightView, 0, sizeof(lightView));
        lightView._11 = sx; lightView._12 = u2x; lightView._13 = fx; lightView._14 = 0.0f;
        lightView._21 = sy; lightView._22 = u2y; lightView._23 = fy; lightView._24 = 0.0f;
        lightView._31 = sz; lightView._32 = u2z; lightView._33 = fz; lightView._34 = 0.0f;
        lightView._41 = -sx * ex - sy * ey - sz * ez;
        lightView._42 = -u2x * ex - u2y * ey - u2z * ez;
        lightView._43 = -fx * ex - fy * ey - fz * ez;
        lightView._44 = 1.0f;
        // Orthographic projection covering the stage, D3D row-major.
        float l = -150.0f, r = 150.0f, b = -150.0f, t = 150.0f, n = 1.0f, f = 500.0f;
        memset(&lightProj, 0, sizeof(lightProj));
        lightProj._11 = 2.0f / (r - l);
        lightProj._22 = 2.0f / (t - b);
        lightProj._33 = 1.0f / (f - n);   // D3D depth [0,1]: n->0, f->1
        lightProj._41 = (l + r) / (l - r);
        lightProj._42 = (t + b) / (b - t);
        lightProj._43 = n / (n - f);
        lightProj._44 = 1.0f;
        // Row-vector world -> view -> projection, uploaded transposed to GLSL.
        Multiply(lightView, lightProj, lightViewProj);
    }

    // OpenCMR2: the sampler cache's key carries the level, so the next draws
    // simply create their samplers at the new one.
    void SetAnisotropy(int level) override
    {
        anisotropy = level < 1 ? 1 : level;
    }

    // OpenCMR2: turns motion blur on or off and sets the previous-frame mix
    // (0..255). Takes effect at the next present.
    void SetMotionBlur(BOOL enabled, int mix) override
    {
        if(motionBlurEnabled!=(enabled!=0))motionBlurHasPrevious=false;
        motionBlurEnabled = enabled != 0;
        motionBlurMix = mix < 0 ? 0 : mix > 255 ? 255 : mix;
    }

    // OpenCMR2: turns bloom on or off and sets the bloom intensity (0..255).
    // Takes effect at the next present.
    void SetBloom(BOOL enabled, int intensity) override
    {
        bloomEnabled = enabled != 0;
        bloomIntensity = intensity < 0 ? 0 : intensity > 255 ? 255 : intensity;
    }

    // OpenCMR2: sets the particle weather type (0=off, 1=rain, 2=snow) and
    // emission intensity (0..255). Takes effect at once.
    void SetParticles(int type, int intensity) override
    {
        particleType = type < 0 ? 0 : type > 2 ? 2 : type;
        particleIntensity = intensity < 0 ? 0 : intensity > 255 ? 255 : intensity;
    }

    // OpenCMR2: sets the directional shadow mode (0=off, 1=on) and depth bias.
    // Takes effect at once.
    void SetShadows(int type, int bias) override
    {
        shadowType = type < 0 ? 0 : type > 1 ? 1 : type;
        shadowBias = std::max(0, std::min(bias, 100)) / 10000.0f;
        // Create shadow map textures lazily (Metal can't sample D24_UNORM_S8_UINT,
        // so shadowMap is a color texture holding depth in its R channel).
        if (shadowType != 0 && (shadowMap == NULL || shadowDepth == NULL) &&
            SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_R32_FLOAT, SDL_GPU_TEXTURETYPE_2D,
                SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER)) {
            SDL_ReleaseGPUTexture(device, shadowMap);
            SDL_ReleaseGPUTexture(device, shadowDepth);
            shadowMap = CreateTexture(SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREFORMAT_R32_FLOAT, shadowSize, shadowSize,
                                      1, 1, SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            shadowDepth = CreateTexture(SDL_GPU_TEXTURETYPE_2D, depthFormat, shadowSize, shadowSize,
                                        1, 1, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);
        }
    }

    void SetSunScreenPosition(float x,float y,BOOL visible) override {
        sunVisible=visible&&std::isfinite(x)&&std::isfinite(y)&&x>=0&&x<=1&&y>=0&&y<=1;
        sunX=x;sunY=y;
    }
    // Car lights the fog pass responds to (video.fog_lights); see game/LightField.h.
    GfxFogLight fogLights[8] = {}; int fogLightCount = 0;
    void SetFogLights(const GfxFogLight *lights,int count) override {
        fogLightCount=0;
        if(lights) for(int i=0;i<count&&i<8;i++){fogLights[i]=lights[i];fogLightCount++;}
    }
    void SetShadowQuality(int resolution,int softness) override {
        int size=resolution<=512?512:resolution>=2048?2048:1024;
        shadowSoftness=std::clamp(softness,0,100)*.04f;
        if(size!=shadowSize){
            shadowSize=size;SDL_ReleaseGPUTexture(device,shadowMap);SDL_ReleaseGPUTexture(device,shadowDepth);
            shadowMap=shadowDepth=nullptr;
            if(shadowType)SetShadows(shadowType,(int)(shadowBias*10000));
        }
    }

    // OpenCMR2: lens grime post-process strength settings.
    void SetChromaticAberration(int strength) override
    {
        chromaticAberration = strength < 0 ? 0 : strength > 100 ? 100 : strength;
    }

    void SetVignette(int strength) override
    {
        vignette = strength < 0 ? 0 : strength > 100 ? 100 : strength;
    }

    void SetFilmGrain(int strength) override
    {
        filmGrain = strength < 0 ? 0 : strength > 100 ? 100 : strength;
    }

    // OpenCMR2: depth of field. `enabled` turns it on/off, `focalDistance`
    // (0..100) is where the focus plane sits in depth, `focalRange` (0..100)
    // is how wide the in-focus band is, `maxBlur` (0..100) is the blur radius.
    void SetDepthOfField(BOOL enabled, int focalDistance, int focalRange, int maxBlur) override
    {
        dofEnabled = enabled != 0;
        dofFocalDistance = focalDistance < 0 ? 0 : focalDistance > 100 ? 100 : focalDistance;
        dofFocalRange = focalRange < 0 ? 0 : focalRange > 100 ? 100 : focalRange;
        dofMaxBlur = maxBlur < 0 ? 0 : maxBlur > 100 ? 100 : maxBlur;
    }

    void SetSpeedFocus(float speed) override
    {
        speedFocusTarget = speedEffectsSuppressed || !(speed > 0.0f) ? 0.0f : speed > kTunnelMaxStrength ? kTunnelMaxStrength : speed;
    }

    void SetSpeedEffectsSuppressed(bool v) override {speedEffectsSuppressed=v;if(v)ClearSpeedEffects();}
    void ClearSpeedEffects() override { speedFocusTarget = speedFocusSmooth = 0.0f; speedStreakTarget = speedStreakSmooth = 0.0f; }
    void ClearVaporTrail() override { vaporTarget = vaporSmooth = 0.0f; vaporHistory.Reset(); vaporHavePrev = vaporHaveDir = false; }   // no fade-out and no stored trail points

    // OpenCMR2: god rays / volumetric lighting. `enabled` turns it on/off,
    // `intensity` (0..255) scales the effect strength.
    void SetVaporTrail(float strength) override { vaporTarget = !(strength > 0.0f) ? 0.0f : strength > 1.0f ? 1.0f : strength; }
    void SetSpeedStreak(float strength) override { speedStreakTarget = speedEffectsSuppressed || !(strength > 0.0f) ? 0.0f : strength > 1.0f ? 1.0f : strength; }

    void SetGodRays(BOOL enabled, int intensity) override
    {
        godRaysEnabled = enabled != 0;
        godRaysIntensity = intensity < 0 ? 0 : intensity > 255 ? 255 : intensity;
    }

    // OpenCMR2: atmosphere fog. `amount` (0..200) scales density, `glow` (0..100) the sun-ward
    // brightening of the fog. See render/fog_math.h.
    void SetPostFog(BOOL enabled, int amount, int glow) override
    {
        fogEnabled = enabled != 0;
        fogAmount = amount < 0 ? 0 : amount > 200 ? 200 : amount;
        fogGlow = glow < 0 ? 0 : glow > 100 ? 100 : glow;
    }
    void SetPostFogClearDistance(int worldUnits) override
    {
        fogClearDistance = worldUnits < 0 ? 0 : worldUnits > 200 ? 200 : worldUnits;
    }

    // OpenCMR2: color grading. `temperature` (-100..100) cool->warm,
    // `tint` (-100..100) green->magenta, `saturation` (0..200) color strength,
    // `contrast` (0..200) contrast around middle gray.
    void SetColorGrading(int temperature, int tint, int saturation, int contrast) override
    {
        colorGradingTemperature = temperature < -100 ? -100 : temperature > 100 ? 100 : temperature;
        colorGradingTint = tint < -100 ? -100 : tint > 100 ? 100 : tint;
        colorGradingSaturation = saturation < 0 ? 0 : saturation > 200 ? 200 : saturation;
        colorGradingContrast = contrast < 0 ? 0 : contrast > 200 ? 200 : contrast;
    }

    // ---- textures ----

    SDL_GPUTextureFormat GpuFormat(int format, bool &cpuDecode)
    {
        cpuDecode = false;
        switch (format) {
        case GFX_FORMAT_BC1:
            if (bcSupported)
                return SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM;
            cpuDecode = true;
            return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        case GFX_FORMAT_BC3:
            if (bcSupported)
                return SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM;
            cpuDecode = true;
            return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        case GFX_FORMAT_BUMP_DUDVL:
            return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        default:
            return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        }
    }

    GpuTexture *Ensure(GfxTexture *t)
    {
        if (t->gpu != NULL)
            return (GpuTexture *)t->gpu;
        GpuTexture *g = new GpuTexture;
        g->format = GpuFormat(t->format, g->cpuDecode);
        bool cube = (t->flags & GFX_TEXTURE_CUBE) != 0;
        bool target = (t->flags & GFX_TEXTURE_RENDER_TARGET) != 0;
        SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        if (target)
            usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        g->texture = CreateTexture(cube ? SDL_GPU_TEXTURETYPE_CUBE : SDL_GPU_TEXTURETYPE_2D, g->format, t->width,
                                   t->height, cube ? 6 : 1, t->mipLevels, usage);
        if (target)
            g->depth = CreateTexture(SDL_GPU_TEXTURETYPE_2D, depthFormat, t->width, t->height, 1, 1,
                                     SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);
        t->gpu = g;
        t->dirty = true;
        return g;
    }

    // Bytes of a level as uploaded (after conversion).
    size_t UploadSize(GfxTexture *t, GpuTexture *g, int w, int h)
    {
        if ((t->format == GFX_FORMAT_BC1 || t->format == GFX_FORMAT_BC3) && !g->cpuDecode)
            return Gfx_LevelSize(t->format, w, h, NULL);
        return (size_t)w * h * 4;
    }

    void ConvertLevel(GfxTexture *t, GpuTexture *g, const uint8_t *src, int w, int h, uint8_t *dst)
    {
        size_t n = (size_t)w * h;
        switch (t->format) {
        case GFX_FORMAT_BGRX8:
            for (size_t i = 0; i < n; i++)
                ((uint32_t *)dst)[i] = ((const uint32_t *)src)[i] | 0xff000000;
            break;
        case GFX_FORMAT_BUMP_DUDVL:
            for (size_t i = 0; i < n; i++) {
                dst[i * 4] = src[i * 3];
                dst[i * 4 + 1] = src[i * 3 + 1];
                dst[i * 4 + 2] = src[i * 3 + 2];
                dst[i * 4 + 3] = 0xff;
            }
            break;
        case GFX_FORMAT_BC1:
        case GFX_FORMAT_BC3:
            if (g->cpuDecode) {
                int blockBytes = t->format == GFX_FORMAT_BC1 ? 8 : 16;
                int bw = (w + 3) / 4, bh = (h + 3) / 4;
                uint32_t block[16];
                for (int by = 0; by < bh; by++)
                    for (int bx = 0; bx < bw; bx++) {
                        Gfx_DecodeBlock(t->format, src + (by * bw + bx) * blockBytes, block);
                        for (int y = 0; y < 4; y++)
                            for (int x = 0; x < 4; x++)
                                if (bx * 4 + x < w && by * 4 + y < h)
                                    ((uint32_t *)dst)[(size_t)(by * 4 + y) * w + bx * 4 + x] = block[y * 4 + x];
                    }
            } else {
                memcpy(dst, src, Gfx_LevelSize(t->format, w, h, NULL));
            }
            break;
        default:
            memcpy(dst, src, n * 4);
            break;
        }
    }

    void UploadTextures(SDL_GPUCommandBuffer *cmd, std::vector<GfxTexture *> &textures)
    {
        size_t total = 0;
        for (GfxTexture *t : textures) {
            if (!t->dirty)
                continue;
            GpuTexture *g = Ensure(t);
            if (g->texture == NULL) {
                t->dirty = false;
                continue;
            }
            int faces = (t->flags & GFX_TEXTURE_CUBE) ? 6 : 1;
            for (int f = 0; f < faces; f++)
                for (int m = 0; m < t->mipLevels; m++)
                    total += UploadSize(t, g, std::max(1, t->width >> m), std::max(1, t->height >> m));
        }
        if (total == 0)
            return;
        SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, (Uint32)total, 0 };
        SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
        uint8_t *map = (uint8_t *)SDL_MapGPUTransferBuffer(device, tb, false);
        struct Pending {
            SDL_GPUTexture *texture;
            int face, level, w, h;
            size_t offset;
        };
        std::vector<Pending> pending;
        size_t offset = 0;
        for (GfxTexture *t : textures) {
            if (!t->dirty)
                continue;
            GpuTexture *g = (GpuTexture *)t->gpu;
            int faces = (t->flags & GFX_TEXTURE_CUBE) ? 6 : 1;
            for (int f = 0; f < faces; f++) {
                for (int m = 0; m < t->mipLevels; m++) {
                    int w = std::max(1, t->width >> m), h = std::max(1, t->height >> m);
                    ConvertLevel(t, g, t->levels[f][m].data(), w, h, map + offset);
                    pending.push_back({ g->texture, f, m, w, h, offset });
                    offset += UploadSize(t, g, w, h);
                }
            }
            t->dirty = false;
        }
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        for (const Pending &p : pending) {
            SDL_GPUTextureTransferInfo src = { tb, (Uint32)p.offset, 0, 0 };
            SDL_GPUTextureRegion dst = {};
            dst.texture = p.texture;
            dst.mip_level = p.level;
            dst.layer = p.face;
            dst.w = p.w;
            dst.h = p.h;
            dst.d = 1;
            SDL_UploadToGPUTexture(copy, &src, &dst, false);
        }
        SDL_EndGPUCopyPass(copy);
        SDL_ReleaseGPUTransferBuffer(device, tb);
    }

    void DestroyTexture(GfxTexture *t) override
    {
        GpuTexture *g = (GpuTexture *)t->gpu;
        if (g == NULL || device == NULL)
            return;
        SDL_ReleaseGPUTexture(device, g->texture);
        SDL_ReleaseGPUTexture(device, g->depth);
        delete g;
        t->gpu = NULL;
    }

    void ReadTexture(GfxTexture *t) override
    {
        GpuTexture *g = (GpuTexture *)t->gpu;
        if (g == NULL || t->format != GFX_FORMAT_BGRX8 && t->format != GFX_FORMAT_BGRA8)
            return;
        int faces = (t->flags & GFX_TEXTURE_CUBE) ? 6 : 1;
        size_t level = (size_t)t->width * t->height * 4;
        SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, (Uint32)(level * faces), 0 };
        SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        for (int f = 0; f < faces; f++) {
            SDL_GPUTextureRegion src = {};
            src.texture = g->texture;
            src.layer = f;
            src.w = t->width;
            src.h = t->height;
            src.d = 1;
            SDL_GPUTextureTransferInfo dst = { tb, (Uint32)(level * f), 0, 0 };
            SDL_DownloadFromGPUTexture(copy, &src, &dst);
        }
        SDL_EndGPUCopyPass(copy);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        SDL_WaitForGPUFences(device, true, &fence, 1);
        SDL_ReleaseGPUFence(device, fence);
        const uint8_t *map = (const uint8_t *)SDL_MapGPUTransferBuffer(device, tb, false);
        for (int f = 0; f < faces; f++)
            memcpy(t->levels[f][0].data(), map + level * f, level);
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_ReleaseGPUTransferBuffer(device, tb);
    }

    // ---- pipelines and samplers ----

    // A draw's pipeline: everything of it the state can change.
    static uint32_t PipelineKey(const GfxCommand &cmd, const GfxDrawState &s)
    {
        bool depthTest = s.depthTest;
        return (cmd.transformed ? 1u : 0u) | (uint32_t)(cmd.primitive & 7) << 1 | (s.alphaBlend ? 1u : 0u) << 4 |
               (uint32_t)(s.srcBlend & 15) << 5 | (uint32_t)(s.destBlend & 15) << 9 | (depthTest ? 1u : 0u) << 13 |
               ((depthTest && s.depthWrite) ? 1u : 0u) << 14 | (uint32_t)(s.depthFunc & 15) << 15 |
               (uint32_t)(s.cull & 3) << 19;
    }

    SDL_GPUGraphicsPipeline *Pipeline(uint32_t key)
    {
        auto it = pipelines.find(key);
        if (it != pipelines.end())
            return it->second;
        bool transformed = (key & 1) != 0, alphaBlend = (key >> 4 & 1) != 0, depthTest = (key >> 13 & 1) != 0;
        bool depthWrite = (key >> 14 & 1) != 0;
        int primitive = key >> 1 & 7, srcBlend = key >> 5 & 15, destBlend = key >> 9 & 15, depthFunc = key >> 15 & 15;
        int cull = key >> 19 & 3;

        SDL_GPUVertexBufferDescription vb = {};
        vb.slot = 0;
        vb.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        SDL_GPUVertexAttribute attrs[6] = {};
        int attrCount;
        if (transformed) {
            vb.pitch = sizeof(GfxTLVertex);
            attrs[0] = { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0 };
            attrs[1] = { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 16 };
            attrs[2] = { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 20 };
            attrs[3] = { 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 24 };
            attrCount = 4;
        } else {
            vb.pitch = sizeof(GfxVertex);
            attrs[0] = { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0 };
            attrs[1] = { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 12 };
            attrs[2] = { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 24 };
            attrs[3] = { 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 28 };
            attrs[4] = { 4, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32 };
            attrs[5] = { 5, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 40 };
            attrCount = 6;
        }

        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = alphaBlend;
        color.blend_state.src_color_blendfactor = BlendFactor(srcBlend);
        color.blend_state.dst_color_blendfactor = BlendFactor(destBlend);
        color.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        color.blend_state.src_alpha_blendfactor = BlendFactor(srcBlend);
        color.blend_state.dst_alpha_blendfactor = BlendFactor(destBlend);
        color.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = transformed ? vsTL : vs3d;
        info.fragment_shader = fs;
        info.vertex_input_state.vertex_buffer_descriptions = &vb;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attrs;
        info.vertex_input_state.num_vertex_attributes = attrCount;
        info.primitive_type = PrimitiveType(primitive);
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        // Direct3D's front faces are clockwise; CULL_CCW (its default)
        // removes the back faces.
        info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
        info.rasterizer_state.cull_mode = cull == GFX_CULL_CW    ? SDL_GPU_CULLMODE_FRONT
                                          : cull == GFX_CULL_CCW ? SDL_GPU_CULLMODE_BACK
                                                                   : SDL_GPU_CULLMODE_NONE;
        info.rasterizer_state.enable_depth_clip = true;
        info.depth_stencil_state.enable_depth_test = depthTest;
        info.depth_stencil_state.enable_depth_write = depthTest && depthWrite;
        info.depth_stencil_state.compare_op = CompareOp(depthFunc);
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (p == NULL)
            Sys_Log("gfx: pipeline: %s", SDL_GetError());
        pipelines[key] = p;
        return p;
    }

    SDL_GPUGraphicsPipeline *ClearPipeline(bool color, bool depth)
    {
        uint32_t key = 0x80000000u | (color ? 1u : 0u) | (depth ? 2u : 0u);
        auto it = pipelines.find(key);
        if (it != pipelines.end())
            return it->second;
        SDL_GPUColorTargetDescription target = {};
        target.format = colorFormat;
        target.blend_state.enable_color_write_mask = true;
        target.blend_state.color_write_mask = color ? 0xf : 0;
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsClear;
        info.fragment_shader = fsClear;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = depth;
        info.depth_stencil_state.enable_depth_write = depth;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (p == NULL)
            Sys_Log("gfx: clear pipeline: %s", SDL_GetError());
        pipelines[key] = p;
        return p;
    }

    SDL_GPUGraphicsPipeline *MotionBlurPipeline()
    {
        if (motionBlurPipeline != nullptr)
            return motionBlurPipeline;
        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = false;
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsMotionBlur;
        info.fragment_shader = fsMotionBlur;
        info.vertex_input_state.vertex_buffer_descriptions = nullptr;
        info.vertex_input_state.num_vertex_buffers = 0;
        info.vertex_input_state.vertex_attributes = nullptr;
        info.vertex_input_state.num_vertex_attributes = 0;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = false;
        info.depth_stencil_state.enable_depth_write = false;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = false;
        motionBlurPipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (motionBlurPipeline == NULL)
            Sys_Log("gfx: motion blur pipeline: %s", SDL_GetError());
        return motionBlurPipeline;
    }

    // A full-screen quad pipeline with no vertex buffer, no depth test and no
    // blending: used by the bright-pass, blur and composite bloom passes.
    SDL_GPUGraphicsPipeline *PostProcessPipeline(SDL_GPUShader *vs, SDL_GPUShader *fs)
    {
        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = false;
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vs;
        info.fragment_shader = fs;
        info.vertex_input_state.vertex_buffer_descriptions = nullptr;
        info.vertex_input_state.num_vertex_buffers = 0;
        info.vertex_input_state.vertex_attributes = nullptr;
        info.vertex_input_state.num_vertex_attributes = 0;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = false;
        info.depth_stencil_state.enable_depth_write = false;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = false;
        SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (p == NULL)
            Sys_Log("gfx: post-process pipeline: %s", SDL_GetError());
        return p;
    }

    SDL_GPUGraphicsPipeline *BloomBrightPassPipeline()
    {
        if (bloomBrightPassPipeline != nullptr)
            return bloomBrightPassPipeline;
        bloomBrightPassPipeline = PostProcessPipeline(vsMotionBlur, fsBrightPass);
        if (bloomBrightPassPipeline == NULL)
            Sys_Log("gfx: bloom bright-pass pipeline: %s", SDL_GetError());
        return bloomBrightPassPipeline;
    }

    SDL_GPUGraphicsPipeline *BloomBlurPipeline()
    {
        if (bloomBlurPipeline != nullptr)
            return bloomBlurPipeline;
        bloomBlurPipeline = PostProcessPipeline(vsMotionBlur, fsBlur);
        if (bloomBlurPipeline == NULL)
            Sys_Log("gfx: bloom blur pipeline: %s", SDL_GetError());
        return bloomBlurPipeline;
    }

    SDL_GPUGraphicsPipeline *BloomCompositePipeline()
    {
        if (bloomCompositePipeline != nullptr)
            return bloomCompositePipeline;
        bloomCompositePipeline = PostProcessPipeline(vsMotionBlur, fsComposite);
        if (bloomCompositePipeline == NULL)
            Sys_Log("gfx: bloom composite pipeline: %s", SDL_GetError());
        return bloomCompositePipeline;
    }

    // Runs one full-screen quad pass: clears the target, binds the pipeline,
    // pushes one fragment uniform buffer slot and up to 7 sampler bindings,
    // draws three vertices, ends the pass.
    bool PostProcessPass(SDL_GPUCommandBuffer *cmd, SDL_GPUGraphicsPipeline *pipeline,
                         SDL_GPUTexture *target, const SDL_GPUTextureSamplerBinding *bindings,
                         int numBindings, const void *uniformData, uint32_t uniformSize,int width=0,int height=0)
    {
        SDL_GPURenderPass *pass = Gfx_BeginPostProcessPass(cmd,pipeline,target,bindings,numBindings);
        if (pass == NULL)
            return false;
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        if(width<=0)width=backWidth;if(height<=0)height=backHeight;
        SDL_GPUViewport vp = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
        SDL_Rect scissor = { 0, 0, width, height };
        SDL_SetGPUViewport(pass, &vp);
        SDL_SetGPUScissor(pass, &scissor);
        if (uniformData != NULL)
            SDL_PushGPUFragmentUniformData(cmd, 0, uniformData, uniformSize);
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, numBindings);
        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
        SDL_EndGPURenderPass(pass);
        return true;
    }

    bool PrepareDustResources(){
        if (!fsDustVolume) fsDustVolume=Shader(SHADER_CODE(dust_volume_frag),SDL_GPU_SHADERSTAGE_FRAGMENT,1,1);
        if (!dustVolumePipeline && fsDustVolume) dustVolumePipeline=PostProcessPipeline(vsMotionBlur,fsDustVolume);
        int volumeWidth=std::max(1,backWidth/2),volumeHeight=std::max(1,backHeight/2);
        if (!dustVolumeTarget && dustVolumePipeline) dustVolumeTarget=CreateTexture(SDL_GPU_TEXTURETYPE_2D,colorFormat,
            volumeWidth,volumeHeight,1,1,SDL_GPU_TEXTUREUSAGE_COLOR_TARGET|SDL_GPU_TEXTUREUSAGE_SAMPLER);
        if (!fsDustComposite) fsDustComposite=Shader(SHADER_CODE(dust_composite_frag),SDL_GPU_SHADERSTAGE_FRAGMENT,3,1);
        if (!dustCompositePipeline && fsDustComposite) dustCompositePipeline=PostProcessPipeline(vsMotionBlur,fsDustComposite);
        if (!dustCompositeTarget && dustCompositePipeline) dustCompositeTarget=CreateTexture(SDL_GPU_TEXTURETYPE_2D,colorFormat,
            backWidth,backHeight,1,1,SDL_GPU_TEXTUREUSAGE_COLOR_TARGET|SDL_GPU_TEXTUREUSAGE_SAMPLER);
        return dustVolumeTarget && dustVolumePipeline && dustCompositeTarget && dustCompositePipeline;
    }

    void RenderDustVolume(SDL_GPUCommandBuffer *cmd,const GfxCommand &command,const GfxDrawState &state,const GfxFrame &frame)
    {
        if (!SupportsDustVolume() || command.dustCount<=0) return;
        // First prototype supports one full-screen perspective viewport only.
        // Split-screen/cubemap/orthographic paths keep ordinary wheel effects.
        GfxViewport dustViewport=ToTarget(state.viewport,state.screenW,state.screenH,true,backWidth,backHeight);
        if (dustViewport.x || dustViewport.y || dustViewport.width!=(DWORD)backWidth ||
            dustViewport.height!=(DWORD)backHeight || state.projection._34==0) return;
        int volumeWidth=std::max(1,backWidth/2),volumeHeight=std::max(1,backHeight/2);
        struct Uniforms {float inverse[16],camera[4],target[4],centres[32][4],colours[32][4];} u={};
        GfxMatrix vp;Multiply(state.view,state.projection,&vp._11);
        float inverseView[16];
        if (!DustVolume_Inverse(vp,u.inverse) || !DustVolume_Inverse(state.view,inverseView)) return;
        u.camera[0]=inverseView[12];u.camera[1]=inverseView[13];u.camera[2]=inverseView[14];
        u.camera[3]=(float)command.dustCount;u.target[0]=1.f/volumeWidth;u.target[1]=1.f/volumeHeight;
        u.target[2]=1.f;u.target[3]=200.f;
        if(command.dustOffset+command.dustCount>frame.dustLobes.size())return;
        for(int i=0;i<command.dustCount;++i){const auto &lobe=frame.dustLobes[command.dustOffset+i];memcpy(u.centres[i],lobe.centreRadius,16);memcpy(u.colours[i],lobe.colourDensity,16);}
        GfxStageState linear;linear.address=GFX_ADDRESS_CLAMP;linear.minFilter=linear.magFilter=GFX_FILTER_LINEAR;
        GfxStageState point;point.address=GFX_ADDRESS_CLAMP;
        SDL_GPUTextureSamplerBinding binding={backDepth,Sampler(point)};
        if(!PostProcessPass(cmd,dustVolumePipeline,dustVolumeTarget,&binding,1,&u,sizeof(u),volumeWidth,volumeHeight))return;
        SDL_GPUTextureSamplerBinding composite[3]={{backColor,Sampler(point)},{dustVolumeTarget,Sampler(point)},{backDepth,Sampler(point)}};
        float size[4]={1.f/backWidth,1.f/backHeight,0,0};
        if(!PostProcessPass(cmd,dustCompositePipeline,dustCompositeTarget,composite,3,size,sizeof(size)))return;
        SDL_GPUBlitInfo blit={};blit.source.texture=dustCompositeTarget;blit.source.w=backWidth;blit.source.h=backHeight;
        blit.destination.texture=backColor;blit.destination.w=backWidth;blit.destination.h=backHeight;
        blit.load_op=SDL_GPU_LOADOP_DONT_CARE;blit.filter=SDL_GPU_FILTER_NEAREST;SDL_BlitGPUTexture(cmd,&blit);
        if(!dustVolumeLogged){Sys_Log("options: world dust volume encoded after world depth, before HUD; dry wheel sheets suppressed, max32 terrain-anchored clouds");dustVolumeLogged=true;}
    }

    // Billboard particle pipeline: position+colour+size vertices, alpha
    // blending, no depth test, full screen.
    SDL_GPUGraphicsPipeline *ParticlePipeline()
    {
        if (particlePipeline != nullptr)
            return particlePipeline;
        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = true;
        color.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        color.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        color.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        color.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        color.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        color.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        SDL_GPUVertexBufferDescription vb = {};
        vb.slot = 0;
        vb.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vb.pitch = sizeof(float) * 2 + 4 + sizeof(float) * 2;  // pos, colour, size
        SDL_GPUVertexAttribute attrs[3] = {};
        attrs[0] = { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0 };
        attrs[1] = { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 8 };
        attrs[2] = { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 12 };
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsParticle;
        info.fragment_shader = fsParticle;
        info.vertex_input_state.vertex_buffer_descriptions = &vb;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attrs;
        info.vertex_input_state.num_vertex_attributes = 3;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = false;
        info.depth_stencil_state.enable_depth_write = false;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = false;
        particlePipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (particlePipeline == NULL)
            Sys_Log("gfx: particle pipeline: %s", SDL_GetError());
        return particlePipeline;
    }

    // Depth-only shadow map pipeline: transforms by lightViewProj * world,
    // writes depth to shadowDepth and depth value to shadowMap's R channel.
    SDL_GPUGraphicsPipeline *ShadowPipeline()
    {
        if (shadowPipeline != nullptr)
            return shadowPipeline;
        SDL_GPUColorTargetDescription color = {};
        color.format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
        color.blend_state.enable_blend = false;
        color.blend_state.color_write_mask = 0xf;   // write depth to shadowMap's R channel
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsShadow;
        info.fragment_shader = fsShadow;
        SDL_GPUVertexBufferDescription vb = {};
        vb.slot = 0; vb.pitch = sizeof(GfxVertex); vb.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        SDL_GPUVertexAttribute attributes[3] = {{0,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,0},{2,0,SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,24},{4,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,32}};
        info.vertex_input_state.vertex_buffer_descriptions = &vb;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attributes;
        info.vertex_input_state.num_vertex_attributes = 3;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = true;
        info.depth_stencil_state.enable_depth_write = true;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        shadowPipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (shadowPipeline == NULL)
            Sys_Log("gfx: shadow pipeline: %s", SDL_GetError());
        return shadowPipeline;
    }

    // Full-screen lens grime pipeline: chromatic aberration, vignetting,
    // film grain; reads the scene texture and writes the final frame.
    SDL_GPUGraphicsPipeline *LensGrimePipeline()
    {
        if (lensGrimePipeline != nullptr)
            return lensGrimePipeline;
        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = false;
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsMotionBlur;
        info.fragment_shader = fsLensGrime;
        info.vertex_input_state.vertex_buffer_descriptions = nullptr;
        info.vertex_input_state.num_vertex_buffers = 0;
        info.vertex_input_state.vertex_attributes = nullptr;
        info.vertex_input_state.num_vertex_attributes = 0;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = false;
        info.depth_stencil_state.enable_depth_write = false;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        lensGrimePipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (lensGrimePipeline == NULL)
            Sys_Log("gfx: lens grime pipeline: %s", SDL_GetError());
        return lensGrimePipeline;
    }

    SDL_GPUGraphicsPipeline *SpeedStreakPipeline()
    {
        if (speedStreakPipeline != nullptr)
            return speedStreakPipeline;
        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = false;
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsMotionBlur;
        info.fragment_shader = fsSpeedStreak;
        info.vertex_input_state.vertex_buffer_descriptions = nullptr;
        info.vertex_input_state.num_vertex_buffers = 0;
        info.vertex_input_state.vertex_attributes = nullptr;
        info.vertex_input_state.num_vertex_attributes = 0;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = false;
        info.depth_stencil_state.enable_depth_write = false;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        speedStreakPipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (speedStreakPipeline == NULL)
            Sys_Log("gfx: speed streak pipeline: %s", SDL_GetError());
        return speedStreakPipeline;
    }

    // Depth of field pipeline: samples scene + depth map, applies blur based
    // on depth distance from focal plane.
    SDL_GPUGraphicsPipeline *DofPipeline()
    {
        if (dofPipeline != nullptr)
            return dofPipeline;
        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = false;
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsMotionBlur;
        info.fragment_shader = fsDoF;
        info.vertex_input_state.vertex_buffer_descriptions = nullptr;
        info.vertex_input_state.num_vertex_buffers = 0;
        info.vertex_input_state.vertex_attributes = nullptr;
        info.vertex_input_state.num_vertex_attributes = 0;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = false;
        info.depth_stencil_state.enable_depth_write = false;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        dofPipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (dofPipeline == NULL)
            Sys_Log("gfx: dof pipeline: %s", SDL_GetError());
        return dofPipeline;
    }

    // God rays pipeline: ray-marches toward the light source, accumulating
    // light where not occluded by depth.
    SDL_GPUGraphicsPipeline *GodRaysPipeline()
    {
        if (godRaysPipeline != nullptr)
            return godRaysPipeline;
        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = false;
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsMotionBlur;
        info.fragment_shader = fsGodRays;
        info.vertex_input_state.vertex_buffer_descriptions = nullptr;
        info.vertex_input_state.num_vertex_buffers = 0;
        info.vertex_input_state.vertex_attributes = nullptr;
        info.vertex_input_state.num_vertex_attributes = 0;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = false;
        info.depth_stencil_state.enable_depth_write = false;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        godRaysPipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (godRaysPipeline == NULL)
            Sys_Log("gfx: god rays pipeline: %s", SDL_GetError());
        return godRaysPipeline;
    }

    // Atmosphere fog pipeline: full-screen scene+depth pass, no blending (fog.frag).
    SDL_GPUGraphicsPipeline *FogPipeline()
    {
        if (fogPipeline != nullptr)
            return fogPipeline;
        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = false;
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsMotionBlur;
        info.fragment_shader = fsFog;
        info.vertex_input_state.vertex_buffer_descriptions = nullptr;
        info.vertex_input_state.num_vertex_buffers = 0;
        info.vertex_input_state.vertex_attributes = nullptr;
        info.vertex_input_state.num_vertex_attributes = 0;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = false;
        info.depth_stencil_state.enable_depth_write = false;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        fogPipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (fogPipeline == NULL)
            Sys_Log("gfx: fog pipeline: %s", SDL_GetError());
        return fogPipeline;
    }

    SDL_GPUShader *fsSunGlare=nullptr;
    SDL_GPUGraphicsPipeline *sunGlarePipeline=nullptr;
    // Color grading pipeline: applies temperature, tint, saturation, contrast.
    SDL_GPUGraphicsPipeline *ColorGradingPipeline()
    {
        if (colorGradingPipeline != nullptr)
            return colorGradingPipeline;
        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = false;
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsMotionBlur;
        info.fragment_shader = fsColorGrading;
        info.vertex_input_state.vertex_buffer_descriptions = nullptr;
        info.vertex_input_state.num_vertex_buffers = 0;
        info.vertex_input_state.vertex_attributes = nullptr;
        info.vertex_input_state.num_vertex_attributes = 0;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = false;
        info.depth_stencil_state.enable_depth_write = false;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        colorGradingPipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (colorGradingPipeline == NULL)
            Sys_Log("gfx: color grading pipeline: %s", SDL_GetError());
        return colorGradingPipeline;
    }

    // ---- particles ----

    void EmitParticle(int type, float x, float y, float vx, float vy, float sx, float sy,
                      float r, float g, float b, float a, float life)
    {
        for (int i = 0; i < kMaxParticles; i++) {
            if (!particles[i].active) {
                particles[i] = { x, y, vx, vy, sx, sy, r, g, b, a, life, true };
                return;
            }
        }
    }

    void UpdateParticles(float dt)
    {
        if (particleType == 0 || particleIntensity <= 0 || dt <= 0.0f)
            return;
        float emitPerSec = 0.0f;
        if (particleType == 1) {   // rain
            emitPerSec = 400.0f * (particleIntensity / 255.0f);
        } else if (particleType == 2) {  // snow
            emitPerSec = 120.0f * (particleIntensity / 255.0f);
        }
        int toEmit = (int)(emitPerSec * dt);
        static float carry = 0.0f;
        carry += emitPerSec * dt - toEmit;
        if (carry >= 1.0f) {
            toEmit += (int)carry;
            carry -= (float)(int)carry;
        }
        for (int e = 0; e < toEmit; e++) {
            static uint32_t seed = 0x12345678;
            seed = seed * 1664525u + 1013904223u;
            float rnd = (seed & 0x7fffffff) / (float)0x7fffffff;
            seed = seed * 1664525u + 1013904223u;
            float rnd2 = (seed & 0x7fffffff) / (float)0x7fffffff;
            if (particleType == 1) {
                float x = rnd;
                float y = -0.05f - rnd2 * 0.1f;
                float vy = 0.9f + rnd * 1.2f;
                EmitParticle(1, x, y, 0.0f, vy, 0.0015f, 0.018f,
                             0.7f, 0.8f, 1.0f, 0.35f, 0.25f + rnd2 * 0.2f);
            } else if (particleType == 2) {
                float x = rnd;
                float y = -0.05f - rnd2 * 0.1f;
                float vy = 0.15f + rnd * 0.25f;
                float vx = (rnd2 - 0.5f) * 0.15f;
                EmitParticle(2, x, y, vx, vy, 0.008f, 0.008f,
                             0.95f, 0.98f, 1.0f, 0.5f, 1.5f + rnd * 2.0f);
            }
        }
        int count = 0;
        for (int i = 0; i < kMaxParticles; i++) {
            Particle &p = particles[i];
            if (!p.active)
                continue;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.life -= dt;
            if (p.life <= 0.0f || p.y > 1.05f || p.x < -0.05f || p.x > 1.05f) {
                p.active = false;
                continue;
            }
            particles[count++] = p;
        }
        particleCount = count;
    }

    bool RenderParticles(SDL_GPUCommandBuffer *cmd)
    {
        if (particleType == 0 || particleCount == 0)
            return false;
        SDL_GPUGraphicsPipeline *pipeline = ParticlePipeline();
        SDL_GPUSampler *sampler = Sampler(GfxStageState());
        if (!cmd || !backColor || !pipeline || !white2D || !sampler) return false;
        uint32_t vertexBytes = (uint32_t)(particleCount * 6 * 20);
        if (vertexBytes > particleVBCapacity) {
            SDL_ReleaseGPUBuffer(device, particleVB);
            particleVBCapacity = std::max(vertexBytes, particleVBCapacity * 2);
            SDL_GPUBufferCreateInfo info = { SDL_GPU_BUFFERUSAGE_VERTEX, particleVBCapacity, 0 };
            particleVB = SDL_CreateGPUBuffer(device, &info);
        }
        if (particleVB == NULL)
            return false;
        SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, vertexBytes, 0 };
        SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
        if (!tb) return false;
        uint8_t *map = (uint8_t *)SDL_MapGPUTransferBuffer(device, tb, true);
        if (!map) { SDL_ReleaseGPUTransferBuffer(device,tb); return false; }
        // Screen-space snow glints (video.snow_glint): flakes twinkle toward the
        // sun and brighten inside projected car light beams (weather_glint.h).
        float glintAmt = particleType == 2 ? std::max(0, std::min(200, Sys_GetOption("video.snow_glint", 100))) / 100.0f : 0.0f;
        float glintAspect = backHeight > 0 ? (float)backWidth / (float)backHeight : 1.0f;
        uint8_t *write = map;
        for (int i = 0; i < particleCount; i++) {
            const Particle &p = particles[i];
            float pr = p.r, pg = p.g, pb = p.b;
            if (glintAmt > 0.0f) {
                float prox = WeatherGlint_SunProximity(p.x, p.y, sunX, sunY, sunVisible ? 1 : 0, glintAspect);
                float m = WeatherGlint_Twinkle(WeatherGlint_Phase(i, p.life), prox);
                m = 1.0f + (m - 1.0f) * glintAmt;
                pr *= m; pg *= m; pb *= m;
                for (int li = 0; li < fogLightCount; ++li) {
                    const GfxFogLight *L = &fogLights[li];
                    WeatherGlint_LightBoost(p.x, p.y, glintAspect, L->x, L->y, L->intensity, L->r, L->g, L->b, &pr, &pg, &pb);
                }
                if (pr > 1.0f) pr = 1.0f;
                if (pg > 1.0f) pg = 1.0f;
                if (pb > 1.0f) pb = 1.0f;
            }
            uint8_t r = (uint8_t)(pr * 255), g = (uint8_t)(pg * 255);
            uint8_t b = (uint8_t)(pb * 255), a = (uint8_t)(p.a * 255);
            for (int v = 0; v < 6; v++) {
                memcpy(write, &p.x, 8); write += 8;
                memcpy(write, &r, 1); write += 1;
                memcpy(write, &g, 1); write += 1;
                memcpy(write, &b, 1); write += 1;
                memcpy(write, &a, 1); write += 1;
                memcpy(write, &p.sx, 8); write += 8;
            }
        }
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        if (!copy) { SDL_ReleaseGPUTransferBuffer(device,tb); return false; }
        SDL_GPUTransferBufferLocation src = { tb, 0 };
        SDL_GPUBufferRegion dst = { particleVB, 0, vertexBytes };
        SDL_UploadToGPUBuffer(copy, &src, &dst, true);
        SDL_EndGPUCopyPass(copy);
        SDL_ReleaseGPUTransferBuffer(device, tb);
        SDL_GPURenderPass *pass = Gfx_BeginColourOnlyPass(cmd,backColor,SDL_GPU_LOADOP_LOAD);
        if (pass == NULL)
            return false;
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        SDL_GPUViewport vp = { 0.0f, 0.0f, (float)backWidth, (float)backHeight, 0.0f, 1.0f };
        SDL_Rect scissor = { 0, 0, backWidth, backHeight };
        SDL_SetGPUViewport(pass, &vp);
        SDL_SetGPUScissor(pass, &scissor);
        SDL_GPUBufferBinding vbBinding = { particleVB, 0 };
        SDL_BindGPUVertexBuffers(pass, 0, &vbBinding, 1);
        SDL_GPUTextureSamplerBinding bindings[1] = {
            { white2D, sampler },
        };
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 1);
        SDL_DrawGPUPrimitives(pass, (uint32_t)(particleCount * 6), 1, 0, 0);
        SDL_EndGPURenderPass(pass);
        return true;
    }

    SDL_GPUSampler *Sampler(const GfxStageState &stage)
    {
        int aniso = anisotropy;
        uint32_t key = (uint32_t)(stage.minFilter & 3) | (uint32_t)(stage.magFilter & 3) << 2 |
                       (uint32_t)(stage.mipFilter & 3) << 4 | (uint32_t)(stage.address & 3) << 6 |
                       (uint32_t)(aniso & 31) << 8;
        auto it = samplers.find(key);
        if (it != samplers.end())
            return it->second;
        SDL_GPUSamplerCreateInfo info = {};
        info.min_filter = stage.minFilter == GFX_FILTER_LINEAR ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
        info.mag_filter = stage.magFilter == GFX_FILTER_LINEAR ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
        info.mipmap_mode = stage.mipFilter == GFX_MIPFILTER_LINEAR ? SDL_GPU_SAMPLERMIPMAPMODE_LINEAR
                                                                   : SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        SDL_GPUSamplerAddressMode mode = stage.address == GFX_ADDRESS_CLAMP    ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
                                         : stage.address == GFX_ADDRESS_MIRROR ? SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT
                                                                               : SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        info.address_mode_u = info.address_mode_v = info.address_mode_w = mode;
        info.min_lod = 0.0f;
        info.max_lod = stage.mipFilter == GFX_MIPFILTER_NONE ? 0.0f : 1000.0f;
        if (aniso > 1 && stage.mipFilter != GFX_MIPFILTER_NONE) {
            info.enable_anisotropy = true;
            info.max_anisotropy = (float)aniso;
        }
        SDL_GPUSampler *s = SDL_CreateGPUSampler(device, &info);
        samplers[key] = s;
        return s;
    }

    // ---- uniforms ----

    // Whether two states give the same VertexUniforms (FillVertexUniforms).
    static bool SameVertexUniforms(const GfxDrawState &a, const GfxDrawState &b)
    {
        for (int i = 0; i < 2; i++)
            if (a.stages[i].texCoordIndex != b.stages[i].texCoordIndex ||
                a.stages[i].textureTransform != b.stages[i].textureTransform)
                return false;
        return a.viewport.width == b.viewport.width && a.viewport.height == b.viewport.height &&
               a.screenW == b.screenW && a.screenH == b.screenH && a.lighting == b.lighting && a.colorVertex == b.colorVertex && a.specular == b.specular &&
               a.localViewer == b.localViewer && a.normalizeNormals == b.normalizeNormals &&
               a.diffuseSource == b.diffuseSource && a.specularSource == b.specularSource &&
               a.ambientSource == b.ambientSource && a.emissiveSource == b.emissiveSource && a.fog == b.fog &&
               a.fogTableMode == b.fogTableMode && a.fogVertexMode == b.fogVertexMode &&
               a.fogStart == b.fogStart && a.fogEnd == b.fogEnd && a.ambient == b.ambient &&
               memcmp(&a.world, &b.world, sizeof(a.world)) == 0 && memcmp(&a.view, &b.view, sizeof(a.view)) == 0 &&
               memcmp(&a.projection, &b.projection, sizeof(a.projection)) == 0 &&
               memcmp(a.texture, b.texture, sizeof(a.texture)) == 0 &&
               memcmp(&a.material, &b.material, sizeof(a.material)) == 0 &&
               memcmp(a.lightEnabled, b.lightEnabled, sizeof(a.lightEnabled)) == 0 &&
               memcmp(a.lights, b.lights, sizeof(a.lights)) == 0;
    }

    // A rectangle of the screen the game draws in (Gfx_SetVirtualScreen) in
    // the back buffer's pixels, within it; elsewhere as it is.
    static GfxViewport ToTarget(const GfxViewport &v, int screenW, int screenH, bool backBuffer, int targetW,
                                int targetH)
    {
        if (!backBuffer || screenW <= 0 || screenH <= 0)
            return v;
        float kx = (float)targetW / screenW, ky = (float)targetH / screenH;
        int x0 = std::clamp((int)lroundf(v.x * kx), 0, targetW);
        int y0 = std::clamp((int)lroundf(v.y * ky), 0, targetH);
        int x1 = std::clamp((int)lroundf((v.x + v.width) * kx), x0, targetW);
        int y1 = std::clamp((int)lroundf((v.y + v.height) * ky), y0, targetH);
        GfxViewport r = v;
        r.x = x0;
        r.y = y0;
        r.width = x1 - x0;
        r.height = y1 - y0;
        return r;
    }

    void FillVertexUniforms(const GfxCommand &cmd, const GfxDrawState &s, int targetW, int targetH,
                            const GfxViewport &viewport, bool scaled, VertexUniforms &u,
                            float jitterX = 0.0f, float jitterY = 0.0f)
    {
        memset(&u, 0, sizeof(u));
        Multiply(s.world, s.view, u.worldView);
        memcpy(u.world, &s.world, sizeof(u.world));
        memcpy(u.lightViewProj, lightViewProj, sizeof(u.lightViewProj));
        memcpy(u.projection, &s.projection, sizeof(u.projection));
        memcpy(u.textureMatrix[0], &s.texture[0], 64);
        memcpy(u.textureMatrix[1], &s.texture[1], 64);
        Copy4(s.material.diffuse, u.materialDiffuse);
        Copy4(s.material.ambient, u.materialAmbient);
        Copy4(s.material.specular, u.materialSpecular);
        Copy4(s.material.emissive, u.materialEmissive);
        Color(s.ambient, u.globalAmbient);
        u.globalAmbient[3] = s.material.power;
        for (int i = 0; i < 8; i++) {
            const GfxLight &l = s.lights[i];
            LightUniform &lu = u.lights[i];
            Copy4(l.diffuse, lu.diffuse);
            Copy4(l.specular, lu.specular);
            Copy4(l.ambient, lu.ambient);
            TransformPoint(l.position, 1.0f, s.view, lu.position);
            lu.position[3] = (float)l.type;
            TransformPoint(l.direction, 0.0f, s.view, lu.direction);
            lu.direction[3] = l.range > 0.0f ? l.range : 1e30f;
            lu.attenuation[0] = l.attenuation0;
            lu.attenuation[1] = l.attenuation1;
            lu.attenuation[2] = l.attenuation2;
            lu.attenuation[3] = l.falloff;
            lu.spot[0] = cosf(l.theta * 0.5f);
            lu.spot[1] = cosf(l.phi * 0.5f);
            lu.spot[2] = s.lightEnabled[i] ? 1.0f : 0.0f;
        }
        u.lightFlags[0] = s.lighting;
        u.lightFlags[1] = s.colorVertex;
        u.lightFlags[2] = s.specular;
        u.lightFlags[3] = s.localViewer;
        u.sources[0] = s.diffuseSource;
        u.sources[1] = s.specularSource;
        u.sources[2] = s.ambientSource;
        u.sources[3] = s.emissiveSource;
        u.fogFlags[0] = (s.fog && s.fogTableMode == GFX_FOG_NONE) ? s.fogVertexMode : 0;
        u.fogFlags[1] = s.normalizeNormals;
        // Direct3D takes the fog factor from the vertex's specular alpha when
        // fog is on with neither vertex nor table fog: the backdrop of a
        // foggy stage is drawn so, its vertices' alpha 0 filling it with fog.
        u.fogFlags[2] = s.fog && s.fogTableMode == GFX_FOG_NONE && s.fogVertexMode == GFX_FOG_NONE;
        u.fogParams[0] = s.fogStart;
        u.fogParams[1] = s.fogEnd;
        u.texCoordIndex[0] = (int32_t)s.stages[0].texCoordIndex;
        u.texCoordIndex[1] = (int32_t)s.stages[1].texCoordIndex;
        u.texCoordIndex[2] = s.stages[0].textureTransform;
        u.texCoordIndex[3] = s.stages[1].textureTransform;
        // Pre-transformed vertices are in pixels of the game's screen, scaled
        // to the back buffer's.
        u.target[0] = (float)(scaled ? s.screenW : targetW);
        u.target[1] = (float)(scaled ? s.screenH : targetH);
        // Direct3D 7's half-pixel offset of the viewport transform.
        u.target[2] = viewport.width ? 1.0f / viewport.width : 0.0f;
        u.target[3] = viewport.height ? -1.0f / viewport.height : 0.0f;
        // Temporal AA sub-pixel jitter in NDC (2 per viewport extent), world 3D only.
        if (viewport.width) u.target[2] += 2.0f * jitterX / viewport.width;
        if (viewport.height) u.target[3] -= 2.0f * jitterY / viewport.height;
        (void)cmd;
    }

    int TextureKind(const GfxStageState &stage)
    {
        GfxTexture *t = Gfx_DrawnTexture(stage.texture);
        if (t == NULL)
            return 0;
        if (t->flags & GFX_TEXTURE_CUBE)
            return 2;
        if (t->format == GFX_FORMAT_BUMP_DUDVL)
            return 3;
        if (t->flags & GFX_TEXTURE_DISTANCE_FIELD)
            return 4;
        if (t->flags & GFX_TEXTURE_MENU_DOT)
            return 5;
        if (t->flags & GFX_TEXTURE_VECTOR_ICON)
            return 6;
        return 1;
    }

    // A 2D texture drawn with point filtering and no mipmaps (the game's
    // sprites, text and HUD). Scaled, nearest texels would come out one or
    // two pixels wide, or be skipped; the shader filters them smoothly
    // through a linear sampler. At their size it reads them as the point
    // filter does.
    bool ScaledPoint(const GfxStageState &stage)
    {
        int kind = TextureKind(stage);
        return (kind == 1 || kind == 6) && stage.magFilter == GFX_FILTER_POINT && stage.mipFilter == GFX_MIPFILTER_NONE;
    }

    // fade: the draw is a 3D one into the back buffer, which the distance
    // fade applies to.
    void FillFragmentUniforms(const GfxDrawState &s, bool fade, FragmentUniforms &u)
    {
        memset(&u, 0, sizeof(u));
        const GfxStageState &a = s.stages[0], &b = s.stages[1];
        u.stage0[0] = a.colorOp;
        u.stage0[1] = a.colorArg1;
        u.stage0[2] = a.colorArg2;
        u.stage0[3] = a.alphaOp;
        u.stage0b[0] = a.alphaArg1;
        u.stage0b[1] = a.alphaArg2;
        u.stage0b[2] = TextureKind(a);
        u.stage0b[3] = ScaledPoint(a);
        u.stage1[0] = b.colorOp;
        u.stage1[1] = b.colorArg1;
        u.stage1[2] = b.colorArg2;
        u.stage1[3] = b.alphaOp;
        u.stage1b[0] = b.alphaArg1;
        u.stage1b[1] = b.alphaArg2;
        u.stage1b[2] = TextureKind(b);
        u.stage1b[3] = ScaledPoint(b);
        memcpy(u.bodyDirt,s.bodyDirt,sizeof(u.bodyDirt));
        Color(s.textureFactor, u.textureFactor);
        u.alphaTest[0] = s.alphaTest;
        u.alphaTest[1] = s.alphaFunc;
        u.alphaTest[2] = maskDraw ? 1 : 0;
        u.alphaRef[0] = (s.alphaRef & 0xff) / 255.0f;
        Color(s.fogColor, u.fogColor);
        u.fogMode[0] = s.fog;
        u.fogMode[1] = s.fogTableMode;
        u.fogMode[2] = s.specular;
        u.fogParams[0] = s.fogStart;
        u.fogParams[1] = s.fogEnd;
        memcpy(u.bumpMatrix, a.bumpMat, sizeof(u.bumpMatrix));
        u.bumpLuminance[0] = a.bumpLumScale;
        u.bumpLuminance[1] = a.bumpLumOffset;
        if (fade && s.fadeEnd > s.fadeStart) {
            u.distanceFade[0] = s.fadeStart;
            u.distanceFade[1] = s.fadeEnd;
            u.distanceFade[2] = 1.0f;
        }
        u.shadowParams[0] = shadowBias;
        u.shadowParams[2] = shadowSoftness;
        u.shadowParams[1] = shadowType != 0 && shadowMap && shadowDepth && shadowPipeline && fade ? 1.0f : 0.0f;
    }

    // The stages' textures (2D and cube for each), then the backdrop.
    void TextureBindings(const GfxDrawState &s, SDL_GPUTextureSamplerBinding bindings[8])
    {
        for (int i = 0; i < 2; i++) {
            const GfxStageState &stage = s.stages[i];
            GfxTexture *t = Gfx_DrawnTexture(stage.texture);
            GpuTexture *g = t != NULL ? Ensure(t) : NULL;
            GfxStageState sampled = stage;
            if (ScaledPoint(stage) || TextureKind(stage) == 5 || TextureKind(stage) == 6)
                sampled.minFilter = sampled.magFilter = GFX_FILTER_LINEAR;
            // A distance field is read between its texels, and from its mip
            // levels where it is drawn smaller.
            if (TextureKind(stage) == 4) {
                sampled.minFilter = sampled.magFilter = GFX_FILTER_LINEAR;
                sampled.mipFilter = GFX_MIPFILTER_LINEAR;
            }
            SDL_GPUSampler *sampler = Sampler(sampled);
            bool cube = t != NULL && (t->flags & GFX_TEXTURE_CUBE);
            SDL_GPUTexture *texture = g != NULL ? g->texture : NULL;
            bindings[i * 2] = { (texture != NULL && !cube) ? texture : white2D, sampler };
            bindings[i * 2 + 1] = { (texture != NULL && cube) ? texture : whiteCube, sampler };
            GpuTexture *native = t != NULL && (t->flags & GFX_TEXTURE_VECTOR_ICON) ? Ensure(stage.texture) : NULL;
            GfxStageState source = stage;
            if (ScaledPoint(stage)) source.minFilter = source.magFilter = GFX_FILTER_LINEAR;
            bindings[5 + i] = { native != NULL ? native->texture : white2D, Sampler(source) };
        }
        bindings[4] = { backdrop, Sampler(GfxStageState()) };
        // Directional light shadow map, bound for the fixed-function shaders.
        GfxStageState shadowSampler; shadowSampler.address = GFX_ADDRESS_CLAMP;
        bindings[7] = { shadowMap != NULL ? shadowMap : white2D, Sampler(shadowSampler) };
    }

    // What a render pass has bound: each draw sets only what differs from the
    // previous one. Render passes start with nothing bound.
    struct PassBindings {
        SDL_GPUGraphicsPipeline *pipeline = nullptr;
        bool viewportSet = false, texturesSet = false, indicesSet = false;
        SDL_GPUViewport viewport = {};
        SDL_Rect scissor = {};
        SDL_GPUTextureSamplerBinding textures[8] = {};
        SDL_GPUBuffer *vertices = nullptr;
    };

    void SetViewport(SDL_GPURenderPass *pass, PassBindings &bound, const SDL_GPUViewport &vp, const SDL_Rect &scissor)
    {
        if (bound.viewportSet && memcmp(&bound.viewport, &vp, sizeof(vp)) == 0 &&
            memcmp(&bound.scissor, &scissor, sizeof(scissor)) == 0)
            return;
        SDL_SetGPUViewport(pass, &vp);
        SDL_SetGPUScissor(pass, &scissor);
        bound.viewport = vp;
        bound.scissor = scissor;
        bound.viewportSet = true;
    }

    // ---- frame execution ----

    void EnsureBuffers(uint32_t vertexBytes, uint32_t indexBytes)
    {
        if (vertexBytes > vertexCapacity) {
            SDL_ReleaseGPUBuffer(device, vertexBuffer);
            vertexCapacity = std::max(vertexBytes, vertexCapacity * 2);
            SDL_GPUBufferCreateInfo info = { SDL_GPU_BUFFERUSAGE_VERTEX, vertexCapacity, 0 };
            vertexBuffer = SDL_CreateGPUBuffer(device, &info);
        }
        if (indexBytes > indexCapacity) {
            SDL_ReleaseGPUBuffer(device, indexBuffer);
            indexCapacity = std::max(indexBytes, indexCapacity * 2);
            SDL_GPUBufferCreateInfo info = { SDL_GPU_BUFFERUSAGE_INDEX, indexCapacity, 0 };
            indexBuffer = SDL_CreateGPUBuffer(device, &info);
        }
    }

    void UploadGeometry(SDL_GPUCommandBuffer *cmd, GfxFrame &frame)
    {
        uint32_t vertexBytes = (uint32_t)frame.vertices.size();
        uint32_t indexBytes = (uint32_t)(frame.indices.size() * 2);
        if (vertexBytes == 0 && indexBytes == 0)
            return;
        EnsureBuffers(std::max(vertexBytes, 4u), std::max(indexBytes, 4u));
        if (vertexBytes + indexBytes > uploadCapacity) {
            SDL_ReleaseGPUTransferBuffer(device, uploadBuffer);
            uploadCapacity = std::max(vertexBytes + indexBytes, uploadCapacity * 2);
            SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, uploadCapacity, 0 };
            uploadBuffer = SDL_CreateGPUTransferBuffer(device, &tbi);
        }
        SDL_GPUTransferBuffer *tb = uploadBuffer;
        // Cycled: a fresh area while the GPU still reads an earlier frame's.
        uint8_t *map = (uint8_t *)SDL_MapGPUTransferBuffer(device, tb, true);
        memcpy(map, frame.vertices.data(), vertexBytes);
        if (indexBytes)
            memcpy(map + vertexBytes, frame.indices.data(), indexBytes);
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        if (vertexBytes) {
            SDL_GPUTransferBufferLocation src = { tb, 0 };
            SDL_GPUBufferRegion dst = { vertexBuffer, 0, vertexBytes };
            SDL_UploadToGPUBuffer(copy, &src, &dst, true);
        }
        if (indexBytes) {
            SDL_GPUTransferBufferLocation isrc = { tb, vertexBytes };
            SDL_GPUBufferRegion idst = { indexBuffer, 0, indexBytes };
            SDL_UploadToGPUBuffer(copy, &isrc, &idst, true);
        }
        SDL_EndGPUCopyPass(copy);
    }

    // The GPU copies of the vertex buffers the frame draws from, where the
    // game has locked them since.
    void UploadVertexBuffers(SDL_GPUCommandBuffer *cmd, GfxFrame &frame)
    {
        SDL_GPUCopyPass *copy = NULL;
        for (GfxVertexBuffer *b : frame.buffers) {
            GpuVertexBuffer *g = (GpuVertexBuffer *)b->gpu;
            if (g == NULL)
                b->gpu = g = new GpuVertexBuffer;
            uint32_t bytes = (uint32_t)(b->vertices.size() * sizeof(GfxVertex));
            if ((g->version == b->version && g->buffer != NULL) || bytes == 0)
                continue;
            if (bytes > g->capacity) {
                SDL_ReleaseGPUBuffer(device, g->buffer);
                SDL_GPUBufferCreateInfo info = { SDL_GPU_BUFFERUSAGE_VERTEX, bytes, 0 };
                g->buffer = SDL_CreateGPUBuffer(device, &info);
                g->capacity = bytes;
            }
            SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, bytes, 0 };
            SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
            memcpy(SDL_MapGPUTransferBuffer(device, tb, false), b->vertices.data(), bytes);
            SDL_UnmapGPUTransferBuffer(device, tb);
            if (copy == NULL)
                copy = SDL_BeginGPUCopyPass(cmd);
            SDL_GPUTransferBufferLocation src = { tb, 0 };
            SDL_GPUBufferRegion dst = { g->buffer, 0, bytes };
            // Cycling keeps the contents earlier frames still draw from.
            SDL_UploadToGPUBuffer(copy, &src, &dst, true);
            SDL_ReleaseGPUTransferBuffer(device, tb);
            g->version = b->version;
        }
        if (copy != NULL)
            SDL_EndGPUCopyPass(copy);
    }

    void DestroyVertexBuffer(GfxVertexBuffer *b) override
    {
        GpuVertexBuffer *g = (GpuVertexBuffer *)b->gpu;
        if (g == NULL)
            return;
        if (device != NULL)
            SDL_ReleaseGPUBuffer(device, g->buffer);
        delete g;
        b->gpu = NULL;
    }

    // Begins a pass on the target at commands[first]. Clears of the whole
    // target there, before any draw, become the pass's load operations (the
    // GPU's fast clears) instead of drawn quads; *cleared counts them.
    SDL_GPURenderPass *BeginPass(SDL_GPUCommandBuffer *cmd, GfxTexture *target, int &w, int &h,
                                 const std::vector<GfxCommand> &commands, size_t first, size_t *cleared)
    {
        SDL_GPUColorTargetInfo color = {};
        SDL_GPUDepthStencilTargetInfo depth = {};
        color.load_op = SDL_GPU_LOADOP_LOAD;
        color.store_op = SDL_GPU_STOREOP_STORE;
        depth.load_op = SDL_GPU_LOADOP_LOAD;
        depth.store_op = SDL_GPU_STOREOP_STORE;
        depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
        if (target == NULL) {
            color.texture = backColor;
            depth.texture = backDepth;
            w = backWidth;
            h = backHeight;
        } else {
            GfxTexture *root = target->cube != NULL ? target->cube : target;
            GpuTexture *g = Ensure(root);
            if (!g) { *cleared = 0; return nullptr; }
            color.texture = g->texture;
            color.layer_or_depth_plane = target->cube != NULL ? target->face : 0;
            depth.texture = g->depth;
            w = root->width;
            h = root->height;
        }
        *cleared = 0;
        // Geometry pipelines require both targets; never pass a descriptor
        // with a null texture to Metal after allocation failure.
        if (!color.texture || !depth.texture) return nullptr;
        for (size_t i = first; i < commands.size() && commands[i].type == GFX_CMD_CLEAR; i++) {
            const GfxCommand &c = commands[i];
            GfxViewport v = ToTarget(c.clearViewport, c.clearScreenW, c.clearScreenH, target == NULL, w, h);
            if (v.x != 0 || v.y != 0 || v.width != (DWORD)w || v.height != (DWORD)h)
                break;
            if (c.clearFlags & GFX_CLEAR_TARGET) {
                float rgba[4];
                Color(c.clearColor, rgba);
                color.load_op = SDL_GPU_LOADOP_CLEAR;
                color.clear_color = { rgba[0], rgba[1], rgba[2], rgba[3] };
            }
            if (c.clearFlags & GFX_CLEAR_ZBUFFER) {
                depth.load_op = SDL_GPU_LOADOP_CLEAR;
                depth.clear_depth = c.clearZ;
            }
            (*cleared)++;
        }
        return SDL_BeginGPURenderPass(cmd, &color, 1, &depth);
    }

    void Execute(GfxFrame &frame, std::vector<GfxTexture *> &textures, GfxReadback &readback, bool present,
                 bool vsync, bool stretch) override
    {
        if (device == NULL)
            return;
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        if (cmd == NULL) {
            Sys_Log("gfx: %s", SDL_GetError());
            return;
        }
        // Textures used by this frame must exist before the passes start.
        for (const GfxDrawState &s : frame.states)
            for (const GfxStageState &stage : s.stages)
                if (stage.texture != NULL)
                    Ensure(Gfx_DrawnTexture(stage.texture));
        for (const GfxCommand &c : frame.commands)
            if (c.type == GFX_CMD_TARGET && c.target != NULL)
                Ensure(c.target->cube != NULL ? c.target->cube : c.target);
        UploadTextures(cmd, textures);
        UploadGeometry(cmd, frame);
        UploadVertexBuffers(cmd, frame);

        SDL_GPURenderPass *pass = NULL;
        PassBindings bound;
        GfxTexture *target = NULL;
        int targetW = backWidth, targetH = backHeight;
        // The uniforms pushed to this command buffer (they stay for its
        // passes): the state and target size of the vertex ones, and the
        // fragment ones. Clears push their own.
        const GfxDrawState *vertexUniformState = NULL;
        int vertexUniformW = 0, vertexUniformH = 0;
        bool vertexUniformScaled = false;
        bool vertexUniformJitter = false;
        TemporalFrame tf;
        bool temporalJitterOn = false;
        FragmentUniforms fragmentUniforms;
        bool fragmentUniformsSet = false;

        bool shadowEncoded=false;
        // OpenCMR2: directional shadow map — render every shadow caster from
        // the sun's point of view into shadowMap depth, before the normal pass
        // samples it.
        if (shadowType != 0 && (shadowMap == NULL || shadowDepth == NULL))
            SetShadows(shadowType, (int)(shadowBias * 10000));
        if (shadowType != 0 && shadowMap != NULL && shadowDepth != NULL && ShadowPipeline() != NULL) {
            SDL_GPUColorTargetInfo color = {};
            color.texture = shadowMap;
            color.load_op = SDL_GPU_LOADOP_CLEAR;
            color.store_op = SDL_GPU_STOREOP_STORE;
            color.clear_color = { 1.0f, 1.0f, 1.0f, 1.0f };
            SDL_GPUDepthStencilTargetInfo depth = {};
            depth.texture = shadowDepth;
            depth.load_op = SDL_GPU_LOADOP_CLEAR;
            depth.store_op = SDL_GPU_STOREOP_STORE;
            depth.clear_depth = 1.0f;
            depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
            depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
                SDL_GPURenderPass *spass = SDL_BeginGPURenderPass(cmd, &color, 1, &depth);
                if (spass != NULL) {
                    SDL_BindGPUGraphicsPipeline(spass, ShadowPipeline());
                    SDL_GPUViewport svp = { 0.0f, 0.0f, (float)shadowSize, (float)shadowSize, 0.0f, 1.0f };
                    SDL_Rect sscissor = { 0, 0, shadowSize, shadowSize };
                    SDL_SetGPUViewport(spass, &svp);
                    SDL_SetGPUScissor(spass, &sscissor);
                    const GfxDrawState *svUniformState = NULL;
                    GfxTexture *shadowTarget = NULL;
                    for (size_t si = 0; si < frame.commands.size(); si++) {
                        const GfxCommand &sc = frame.commands[si];
                        if (sc.type == GFX_CMD_TARGET) { shadowTarget = sc.target; continue; }
                        if (sc.type != GFX_CMD_DRAW || sc.transformed || shadowTarget != NULL ||
                            sc.primitive != GFX_TRIANGLELIST) continue;
                        const GfxDrawState &ss = frame.states[sc.state];
                        // Reproduce only supported texture-alpha cutouts, never solid
                        // billboard rectangles or translucent glass/smoke.
                        if (!Gfx_ShadowCaster(ss)) continue;
                        bool cutout=Gfx_CutoutShadow(ss);
                        GpuTexture *coverage=cutout?Ensure(Gfx_DrawnTexture(ss.stages[0].texture)):nullptr;
                        if(cutout&&!coverage)continue;
                        GfxStageState sample=ss.stages[0];
                        SDL_GPUTextureSamplerBinding binding={cutout?coverage->texture:white2D,Sampler(sample)};
                        if(!binding.texture||!binding.sampler)continue;
                        SDL_BindGPUFragmentSamplers(spass,0,&binding,1);
                        float settings[4]={cutout?1.f:0.f,(ss.alphaRef&255)/255.f,ss.alphaFunc==GFX_CMP_GREATEREQUAL?1.f:0.f,ss.stages[0].alphaOp==GFX_TOP_MODULATE?1.f:0.f};
                        SDL_PushGPUFragmentUniformData(cmd,0,settings,sizeof(settings));
                        if (svUniformState == NULL || memcmp(&ss.world, &svUniformState->world, sizeof(ss.world)) != 0) {
                            VertexUniforms vu;
                            memset(&vu, 0, sizeof(vu));
                            memcpy(vu.world, &ss.world, sizeof(vu.world));
                            memcpy(vu.lightViewProj, lightViewProj, sizeof(vu.lightViewProj));
                            SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
                            svUniformState = &ss;
                        }
                        SDL_GPUBuffer *vertices = sc.buffer != NULL ? ((GpuVertexBuffer *)sc.buffer->gpu)->buffer : vertexBuffer;
                        if (vertices == NULL) continue;
                        SDL_GPUBufferBinding vb = { vertices, 0 };
                        SDL_BindGPUVertexBuffers(spass, 0, &vb, 1);
                        uint32_t firstVertex = sc.firstVertexByte / sizeof(GfxVertex);
                        if (sc.firstIndex != ~0u) {
                            SDL_GPUBufferBinding ib = { indexBuffer, 0 };
                            SDL_BindGPUIndexBuffer(spass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
                            SDL_DrawGPUIndexedPrimitives(spass, sc.indexCount, 1, sc.firstIndex, (Sint32)firstVertex, 0);
                        } else {
                            SDL_DrawGPUPrimitives(spass, sc.vertexCount, 1, firstVertex, 0);
                        }
                    }
                    SDL_EndGPURenderPass(spass);shadowEncoded=true;
                }
            }

        memset(projVotes, 0, sizeof(projVotes)); projVoteTotal = 0; projVoteRejected = 0; // votes are per rendered frame, never carried over
        size_t worldEnd=frame.commands.size();unsigned boundaries=0;bool raceWorld=false;
        for(size_t i=0;i<frame.commands.size();++i)if(frame.commands[i].type==GFX_CMD_WORLD_END){worldEnd=i;raceWorld=frame.commands[i].raceWorld;++boundaries;}
        // Per-view history is not implemented. Multi-view/UI-only frames skip
        // optional full-screen FX rather than smearing another view's HUD.
        bool worldEffects=present && boundaries==1;
        auto drawCommands=[&](size_t first,size_t last) {
        for (size_t i = first; i < last; i++) {
            const GfxCommand &c = frame.commands[i];
            if (c.type == GFX_CMD_WORLD_END) continue;
            if (maskDraw && c.type != GFX_CMD_DRAW && c.type != GFX_CMD_TARGET) continue;
            if (maskDraw && c.type == GFX_CMD_DRAW && (target != NULL || c.transformed || frame.states[c.state].objectId != maskObject)) continue;
            if (c.type == GFX_CMD_TARGET) {
                if (pass != NULL) {
                    SDL_EndGPURenderPass(pass);
                    pass = NULL;
                }
                target = c.target;
                continue;
            }
            if (c.type == GFX_CMD_DUST_VOLUME) {
                if (target != nullptr || !sampledDepth) continue;
                if (pass) { SDL_EndGPURenderPass(pass);pass=nullptr; }
                bound=PassBindings();vertexUniformState=nullptr;fragmentUniformsSet=false;
                RenderDustVolume(cmd,c,frame.states[c.state],frame);
                continue;
            }
            if (c.type == GFX_CMD_BACKDROP) {
                // A copy pass between render passes; the next draw starts a
                // render pass that loads the back buffer as it is.
                if (target != NULL)
                    continue;
                if (pass != NULL) {
                    SDL_EndGPURenderPass(pass);
                    pass = NULL;
                }
                SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
                SDL_GPUTextureLocation src = {}, dst = {};
                src.texture = backColor;
                dst.texture = backdrop;
                SDL_CopyGPUTextureToTexture(copy, &src, &dst, backWidth, backHeight, 1, false);
                SDL_EndGPUCopyPass(copy);
                continue;
            }
            if (pass == NULL) {
                size_t cleared;
                pass = BeginPass(cmd, target, targetW, targetH, frame.commands, i, &cleared);
                bound = PassBindings();
                if (!pass) continue;
                if (cleared > 0) {
                    i += cleared - 1;
                    continue;
                }
            }
            if (c.type == GFX_CMD_CLEAR) {
                bool colorClear = (c.clearFlags & GFX_CLEAR_TARGET) != 0;
                bool depthClear = (c.clearFlags & GFX_CLEAR_ZBUFFER) != 0;
                if (!colorClear && !depthClear)
                    continue;
                SDL_GPUGraphicsPipeline *pipeline = ClearPipeline(colorClear, depthClear);
                if (pipeline != bound.pipeline) {
                    SDL_BindGPUGraphicsPipeline(pass, pipeline);
                    bound.pipeline = pipeline;
                }
                GfxViewport v = ToTarget(c.clearViewport, c.clearScreenW, c.clearScreenH, target == NULL, targetW,
                                         targetH);
                SDL_GPUViewport vp = { (float)v.x, (float)v.y, (float)v.width, (float)v.height, 0.0f, 1.0f };
                SDL_Rect scissor = { (int)v.x, (int)v.y, (int)v.width, (int)v.height };
                SetViewport(pass, bound, vp, scissor);
                ClearUniforms u = {};
                Color(c.clearColor, u.color);
                u.depth[0] = c.clearZ;
                SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));
                SDL_PushGPUFragmentUniformData(cmd, 0, &u, sizeof(u));
                vertexUniformState = NULL;
                fragmentUniformsSet = false;
                SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
                continue;
            }
            const GfxDrawState &s = frame.states[c.state];
            SDL_GPUGraphicsPipeline *pipeline = Pipeline(maskDraw ? MaskKey(PipelineKey(c, s)) : PipelineKey(c, s));
            if (pipeline == NULL)
                continue;
            if (pipeline != bound.pipeline) {
                SDL_BindGPUGraphicsPipeline(pass, pipeline);
                bound.pipeline = pipeline;
            }
            if (!maskDraw && target == NULL && !c.transformed && s.depthTest && s.depthWrite) VoteProjection(s.projection);
            const bool scaled = target == NULL && s.screenW > 0 && s.screenH > 0;
            const GfxViewport viewport = ToTarget(s.viewport, s.screenW, s.screenH, target == NULL, targetW, targetH);
            SDL_GPUViewport vp;
            if (c.transformed)
                vp = { 0.0f, 0.0f, (float)targetW, (float)targetH, 0.0f, 1.0f };
            else
                vp = { (float)viewport.x, (float)viewport.y, (float)viewport.width, (float)viewport.height,
                       viewport.minZ, viewport.maxZ };
            SDL_Rect scissor = { (int)viewport.x, (int)viewport.y, (int)viewport.width, (int)viewport.height };
            SetViewport(pass, bound, vp, scissor);
            const bool jitterDraw = temporalJitterOn && target == NULL && !c.transformed;
            if (vertexUniformState == NULL || vertexUniformW != targetW || vertexUniformH != targetH ||
                vertexUniformScaled != scaled || vertexUniformJitter != jitterDraw ||
                (vertexUniformState != &s && !SameVertexUniforms(*vertexUniformState, s))) {
                VertexUniforms vu;
                FillVertexUniforms(c, s, targetW, targetH, viewport, scaled, vu,
                                   jitterDraw ? tf.jitterX : 0.0f, jitterDraw ? tf.jitterY : 0.0f);
                SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
                vertexUniformState = &s;
                vertexUniformW = targetW;
                vertexUniformH = targetH;
                vertexUniformScaled = scaled;
                vertexUniformJitter = jitterDraw;
            }
            FragmentUniforms fu;
            FillFragmentUniforms(s, target == NULL && !c.transformed, fu);
            if (!fragmentUniformsSet || memcmp(&fu, &fragmentUniforms, sizeof(fu)) != 0) {
                SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
                fragmentUniforms = fu;
                fragmentUniformsSet = true;
            }
            SDL_GPUTextureSamplerBinding textures[8];
            TextureBindings(s, textures);
            if (!bound.texturesSet || memcmp(textures, bound.textures, sizeof(textures)) != 0) {
                SDL_BindGPUFragmentSamplers(pass, 0, textures, 8);
                memcpy(bound.textures, textures, sizeof(textures));
                bound.texturesSet = true;
            }
            // The buffer from its start; the draw gives its first vertex.
            SDL_GPUBuffer *vertices = c.buffer != NULL ? ((GpuVertexBuffer *)c.buffer->gpu)->buffer : vertexBuffer;
            uint32_t firstVertex = c.firstVertexByte / (c.transformed ? sizeof(GfxTLVertex) : sizeof(GfxVertex));
            if (vertices != bound.vertices) {
                SDL_GPUBufferBinding vb = { vertices, 0 };
                SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
                bound.vertices = vertices;
            }
            if (c.firstIndex != ~0u) {
                if (!bound.indicesSet) {
                    SDL_GPUBufferBinding ib = { indexBuffer, 0 };
                    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
                    bound.indicesSet = true;
                }
                SDL_DrawGPUIndexedPrimitives(pass, c.indexCount, 1, c.firstIndex, (Sint32)firstVertex, 0);
            } else {
                SDL_DrawGPUPrimitives(pass, c.vertexCount, 1, firstVertex, 0);
            }
        }
        if (pass != NULL) { SDL_EndGPURenderPass(pass); pass=nullptr; }

        };
        if (worldEffects && raceWorld && sampledDepth && worldEnd < frame.commands.size() &&
            Sys_GetOption("video.temporal_aa", 0) != 0 &&
            temporalTracker.Prepare(frame.commands[worldEnd], frame.states[frame.commands[worldEnd].state], backWidth, backHeight, tf))
        {
            temporalJitterOn = true;
            for (const GfxCommand &oc : frame.commands)
                if (oc.type == GFX_CMD_DRAW && oc.target == NULL && !oc.transformed) {
                    const GfxDrawState &os = frame.states[oc.state];
                    if (os.objectId) { float p[3] = { os.world._41, os.world._42, os.world._43 }; temporalTracker.AddObject(tf, os.objectId, p); }
                }
        }
        else { temporalTracker.Reset(); taaPrevDepthValid = false; }
        drawCommands(0,worldEffects?worldEnd:frame.commands.size());
        temporalJitterOn = false;
        vertexUniformState = NULL;
        bool maskBypassPost = false; // debug_object_mask only: the post chain would soften/recolor the magenta
        // Debug object mask (opt-in, video.debug_object_mask): repaint the followed car's own draws magenta, depth tested.
        if (worldEffects && raceWorld && worldEnd < frame.commands.size() && frame.commands[worldEnd].playerObject != 0 &&
            Sys_GetOption("video.debug_object_mask", 0) != 0) {
            target = NULL; pass = NULL; maskObject = frame.commands[worldEnd].playerObject; maskDraw = true;
            fragmentUniformsSet = false; vertexUniformState = NULL;
            drawCommands(0, worldEnd);
            maskDraw = false; target = NULL; pass = NULL; fragmentUniformsSet = false; vertexUniformState = NULL;
            maskBypassPost = true;
            static bool loggedMask = false;
            if (!loggedMask) { loggedMask = true; Sys_Log("options: debug object mask repainted draws with objectId %u (car slot+1) in magenta, depth tested; AA/bloom/motion blur/DoF/tunnel/grade are bypassed while this debug option is on; native pixels unverified", maskObject); }
        }

        if (!present) {
            SDL_SubmitGPUCommandBuffer(cmd);
            return;
        }

        // OpenCMR2: particles — simulate and render rain/snow into the scene
        // before post-processing, so bloom and motion blur can pick them up.
        bool precipitationEncoded=false;
        if (worldEffects && raceWorld && particleType != 0 && particleIntensity > 0) {
            uint32_t now = SDL_GetTicks();
            float dt = particleLastTicks == 0 ? 0.0f : (float)(now - particleLastTicks) / 1000.0f;
            particleLastTicks = now;
            if (dt > 0.05f)
                dt = 0.05f;   // spiral-of-death guard
            UpdateParticles(dt);
            precipitationEncoded=RenderParticles(cmd);
        }

        if(!raceWorld){particleCount=0;particleLastTicks=0;motionBlurHasPrevious=false;}

        // OpenCMR2: bloom (bright-pass extract, separable blur, additive
        // composite) followed by motion blur (previous-frame blend).
        unsigned requested=(particleType&&particleIntensity?128:0)|(Sys_GetOption("video.spatial_aa",0)?1:0)|(bloomEnabled?2:0)|(motionBlurEnabled?4:0)|(dofEnabled?8:0)|(godRaysEnabled?16:0)|((colorGradingTemperature||colorGradingTint||colorGradingSaturation!=100||colorGradingContrast!=100)?32:0)|(shadowType?64:0)|(fogEnabled&&fogAmount>0?512:0);
        int metalFXStatus=metalFXSpatial?0:-1;
        unsigned encoded=(shadowEncoded?64:0)|(precipitationEncoded?128:0);
        dofStatus={0,speedFocusTarget,0.0f,0.0f,0.0f};
        SDL_GPUTexture *presentSource = backColor;
        if(worldEffects && !maskBypassPost) {
        // OpenCMR2: opt-in temporal AA resolve (PARTIAL: camera-matrix reprojection,
        // 3x3 clamp, no per-object velocity). Runs first so later passes see its output.
        if (tf.active) {
            bool temporalOK = false;
            if (!fsTemporal)
                fsTemporal = Shader(SHADER_CODE(temporal_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 4, 1);
            if (!temporalPipeline && fsTemporal)
                temporalPipeline = PostProcessPipeline(vsMotionBlur, fsTemporal);
            for (auto &t : taaTex)
                if (!t && temporalPipeline)
                    t = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, backWidth, backHeight, 1, 1,
                                      SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            if (!taaPrevDepth && temporalPipeline)
                taaPrevDepth = CreateTexture(SDL_GPU_TEXTURETYPE_2D, depthFormat, backWidth, backHeight, 1, 1,
                                             SDL_GPU_TEXTUREUSAGE_SAMPLER);
            // No previous depth yet (first frame after create/resize): no history.
            if (taaPrevDepth && !taaPrevDepthValid) { tf.u.params[0] = 0.f; }
            if (temporalPipeline && taaTex[0] && taaTex[1] && taaPrevDepth) {
                int cur = taaIndex, prev = 1 - taaIndex;
                GfxStageState linear;
                linear.address = GFX_ADDRESS_CLAMP;
                linear.minFilter = linear.magFilter = GFX_FILTER_LINEAR;
                GfxStageState point = linear;
                point.minFilter = point.magFilter = GFX_FILTER_POINT;
                SDL_GPUTextureSamplerBinding tb[4] = { { backColor, Sampler(linear) }, { backDepth, Sampler(point) },
                                                       { taaTex[prev], Sampler(linear) }, { taaPrevDepth, Sampler(point) } };
                if (PostProcessPass(cmd, temporalPipeline, taaTex[cur], tb, 4, &tf.u, sizeof(tf.u))) {
                    presentSource = taaTex[cur];
                    taaIndex = prev;
                    temporalTracker.Commit(tf);
                    temporalOK = true;
                    if (SDL_GPUCopyPass *dcopy = SDL_BeginGPUCopyPass(cmd)) {
                        SDL_GPUTextureLocation dsrc = {}, ddst = {};
                        dsrc.texture = backDepth; ddst.texture = taaPrevDepth;
                        SDL_CopyGPUTextureToTexture(dcopy, &dsrc, &ddst, backWidth, backHeight, 1, false);
                        SDL_EndGPUCopyPass(dcopy);
                        taaPrevDepthValid = true;
                    } else taaPrevDepthValid = false;
                    if (!temporalLogged) {
                        Sys_Log("options: temporal AA encoded (PARTIAL stage: camera reprojection + clamp, no object velocity; not MetalFX temporal); native pixels unverified");
                        temporalLogged = true;
                    }
                }
            }
            if (!temporalOK) { temporalTracker.Reset(); taaPrevDepthValid = false; }
        }
        // OpenCMR2: AA is not an upscaler or a temporal reconstruction method.
        // HUD/menu draws follow this world-only chain.
        if (spatialAA) {
            if (fsSpatialAA == nullptr)
                fsSpatialAA = Shader(SHADER_CODE(spatial_aa_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
            if (spatialAAPipeline == nullptr && fsSpatialAA != nullptr)
                spatialAAPipeline = PostProcessPipeline(vsMotionBlur, fsSpatialAA);
            if (spatialAATarget == nullptr && spatialAAPipeline != nullptr)
                spatialAATarget = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat,
                    backWidth, backHeight, 1, 1,
                    SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            if (spatialAATarget != nullptr && spatialAAPipeline != nullptr) {
                GfxStageState stage;
                stage.minFilter = stage.magFilter = GFX_FILTER_LINEAR;
                stage.address = GFX_ADDRESS_CLAMP;
                SDL_GPUTextureSamplerBinding binding = { presentSource, Sampler(stage) };
                float uniforms[4] = { 1.0f / backWidth, 1.0f / backHeight, 0.0f, 0.0f };
                if (binding.sampler != nullptr &&
                    PostProcessPass(cmd, spatialAAPipeline, spatialAATarget, &binding, 1,
                                    uniforms, sizeof(uniforms)))
                    {presentSource = spatialAATarget;encoded|=1;}
                else
                    spatialAA = false;
            } else {
                spatialAA = false;
            }
            if (!spatialAA)
                Sys_Log("gfx: spatial AA unavailable; retaining original output");
        }
        SDL_GPUTexture *cloudMask=white2D;
        if(raceWorld && sampledDepth && Clouds_Enabled()&&Sys_GetOption("video.volumetric_clouds",0)!=0){
            CloudUniforms u={};
            if(CloudUniforms_Build(frame.commands[worldEnd],frame.states[frame.commands[worldEnd].state],backWidth,backHeight,u)){
                if(!fsClouds)fsClouds=Shader(SHADER_CODE(clouds_frag),SDL_GPU_SHADERSTAGE_FRAGMENT,2,1);
                if(!cloudPipeline&&fsClouds)cloudPipeline=PostProcessPipeline(vsMotionBlur,fsClouds);
                if(!cloudTarget&&cloudPipeline)cloudTarget=CreateTexture(SDL_GPU_TEXTURETYPE_2D,colorFormat,backWidth,backHeight,1,1,SDL_GPU_TEXTUREUSAGE_COLOR_TARGET|SDL_GPU_TEXTUREUSAGE_SAMPLER);
                GfxStageState sample;sample.address=GFX_ADDRESS_CLAMP;sample.minFilter=sample.magFilter=GFX_FILTER_LINEAR;
                SDL_GPUTextureSamplerBinding cloudBindings[2]={{presentSource,Sampler(sample)},{backDepth,Sampler(sample)}};
                if(cloudTarget&&cloudPipeline&&PostProcessPass(cmd,cloudPipeline,cloudTarget,cloudBindings,2,&u,sizeof(u))){presentSource=cloudTarget;cloudMask=cloudTarget;
                    if(!cloudLogged){Sys_Log("options: volumetric clouds encoded (48 view / 4 sun samples, early-out density), transmission coupled to rays; native pixels unverified");cloudLogged=true;}
                }
            }
        }
        SDL_GPUTexture *sceneSource = presentSource;

        if (bloomEnabled && bloomIntensity > 0 && backColor != NULL && bloomBuf1 != NULL &&
            bloomBuf2 != NULL) {
            // Chain only complete passes. Failed resources retain sceneSource.
            bool bloomOK = true;
            // Bright pass: extract pixels brighter than a threshold -> bloomBuf1.
            {
                float uniforms[2] = { 0.75f, 0.3f };   // threshold, softness
                SDL_GPUTextureSamplerBinding bindings[2] = {
                    { sceneSource, Sampler(GfxStageState()) },
                    { white2D, Sampler(GfxStageState()) },
                };
                bloomOK = PostProcessPass(cmd, BloomBrightPassPipeline(), bloomBuf1, bindings, 2,
                                uniforms, sizeof(uniforms));
            }
            // Separable blur: horizontal then vertical, two ping-pong iterations.
            for (int iteration = 0; bloomOK && iteration < 2; iteration++) {
                struct BlurUniform {float x,y,scale;int samples;float pad[4];};
                BlurUniform hUniforms = {1,0,1,3,{}};   // direction (1,0), offsetScale, samples
                SDL_GPUTextureSamplerBinding hBindings[2] = {
                    { bloomBuf1, Sampler(GfxStageState()) },
                    { white2D, Sampler(GfxStageState()) },
                };
                bloomOK = PostProcessPass(cmd, BloomBlurPipeline(), bloomBuf2, hBindings, 2,
                                &hUniforms, sizeof(hUniforms));

                BlurUniform vUniforms = {0,1,1,3,{}};   // direction (0,1)
                SDL_GPUTextureSamplerBinding vBindings[2] = {
                    { bloomBuf2, Sampler(GfxStageState()) },
                    { white2D, Sampler(GfxStageState()) },
                };
                bloomOK = bloomOK && PostProcessPass(cmd, BloomBlurPipeline(), bloomBuf1, vBindings, 2,
                                &vUniforms, sizeof(vUniforms));
            }
            // Composite: scene + blurred bright pixels -> motionBlurTemp.
            {
                float intensity = bloomIntensity / 255.0f;
                SDL_GPUTextureSamplerBinding bindings[2] = {
                    { sceneSource, Sampler(GfxStageState()) },
                    { bloomBuf1, Sampler(GfxStageState()) },
                };
                if (bloomOK && PostProcessPass(cmd, BloomCompositePipeline(), motionBlurTemp, bindings, 2,
                                    &intensity, sizeof(intensity)))
                    {presentSource = motionBlurTemp;encoded|=2;}
            }
        }

        // Motion blur: blend the current frame (plain or bloomed) with the
        // previous one, then remember it for the next frame.
        if (motionBlurEnabled && motionBlurMix > 0 && motionBlurPrevious != NULL) {
            SDL_GPUTexture *output =
                (presentSource == motionBlurTemp && backColor != NULL) ? backColor : motionBlurTemp;
            float mix = motionBlurMix / 255.0f;
            SDL_GPUTextureSamplerBinding bindings[2] = {
                { presentSource != NULL ? presentSource : white2D, Sampler(GfxStageState()) },
                { motionBlurPrevious, Sampler(GfxStageState()) },
            };
            if (!motionBlurHasPrevious) {
                SDL_GPUCopyPass *copy=SDL_BeginGPUCopyPass(cmd);
                if(copy){SDL_GPUTextureLocation src={},dst={};src.texture=presentSource;dst.texture=motionBlurPrevious;
                    SDL_CopyGPUTextureToTexture(copy,&src,&dst,backWidth,backHeight,1,false);SDL_EndGPUCopyPass(copy);motionBlurHasPrevious=true;}
            } else if (PostProcessPass(cmd, MotionBlurPipeline(), output, bindings, 2, &mix, sizeof(mix))) {
                encoded|=4;
                SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
                if (copy != nullptr) {
                    SDL_GPUTextureLocation src = {}, dst = {};
                    src.texture = output;
                    dst.texture = motionBlurPrevious;
                    SDL_CopyGPUTextureToTexture(copy, &src, &dst, backWidth, backHeight, 1, false);
                    SDL_EndGPUCopyPass(copy);
                    motionBlurHasPrevious = true;
                }
                presentSource = output;
            }
        } else {
            motionBlurHasPrevious = false;
        }

        // OpenCMR2: lens grime — chromatic aberration, vignette, film grain —
        // applied as a final full-screen pass before present.
        if ((chromaticAberration > 0 || vignette > 0 || filmGrain > 0) &&
            presentSource != NULL) {
            float uniforms[4] = { chromaticAberration / 100.0f,
                                  vignette / 100.0f,
                                  filmGrain / 100.0f, 0.0f };
            SDL_GPUTextureSamplerBinding bindings[2] = {
                { presentSource, Sampler(GfxStageState()) },
                { white2D, Sampler(GfxStageState()) },
            };
            SDL_GPUTexture *lensTarget = presentSource == motionBlurTemp ? bloomBuf2 : motionBlurTemp;
            if (PostProcessPass(cmd, LensGrimePipeline(), lensTarget, bindings, 2,
                                uniforms, sizeof(uniforms)))
                presentSource = lensTarget;
        }

        // OpenCMR2: effects consume the previous result, never their output target.
        GfxStageState linearStage;
        linearStage.minFilter = linearStage.magFilter = GFX_FILTER_LINEAR;
        linearStage.address = GFX_ADDRESS_CLAMP;
        GfxStageState depthStage;
        depthStage.address = GFX_ADDRESS_CLAMP;
        SDL_GPUSampler *linearSampler = Sampler(linearStage);
        SDL_GPUSampler *depthSampler = Sampler(depthStage);
        // Atmosphere fog (video.fog): one scene+depth pass, before DoF/god rays so blur
        // and shafts stack on the fogged scene. Uses the same projection votes as DoF;
        // unknown projection falls back to a raw-depth ramp inside the shader.
        if (raceWorld && fogEnabled && fogAmount > 0 && sampledDepth && backDepth != nullptr &&
            fogBuf1 != nullptr && FogPipeline() != nullptr) {
            ResolveProjection(&sceneNear, &sceneFar);
            Gfx_SetFogDepthStatus(GfxFogDepthStatus{ lastResolveDistinct == 0 ? 3 : lastResolveDistinct == 1 ? 1 : 2,
                lastResolveDistinct, lastResolveVotes, lastResolveNear, lastResolveFar });
            // FogUniforms: packed by the single shared packer (render/fog_uniforms.h).
            FogUniformParams fogP;
            fogP.density = Fog_Density(fogAmount);
            fogP.glow = fogGlow / 100.0f;
            fogP.nearZ = sceneNear; fogP.farZ = sceneFar;
            fogP.sunX = sunX; fogP.sunY = sunY;
            fogP.sunVisible = sunVisible ? 1.0f : 0.0f;
            fogP.aspect = backHeight ? (float)backWidth / (float)backHeight : 1.0f;
            fogP.clearDistance = (float)fogClearDistance;
            fogP.lightStrength = std::max(0, std::min(200, Sys_GetOption("video.fog_lights", 100))) / 100.0f;
            fogP.depthHalfRange = 0.0f;   // SDL_GPU depth is full-range
            float fogUniforms[kFogUniformFloats];
            FogUniforms_Pack(fogUniforms, fogP, fogLights, fogLightCount);
            SDL_GPUTextureSamplerBinding fogBindings[2] = {
                { presentSource, linearSampler }, { backDepth, depthSampler }
            };
            if (PostProcessPass(cmd, FogPipeline(), fogBuf1, fogBindings, 2,
                                fogUniforms, sizeof(fogUniforms)))
                { presentSource = fogBuf1; encoded |= 512; }
        }
        // Tunnel vision is independent of the depth blur (see render/dof_gate.h): it still runs with DoF off or dof_blur 0.
        DofGate dofGate = Dof_Gate(dofEnabled, dofFocalRange, dofMaxBlur, speedFocusTarget, speedFocusSmooth, sampledDepth && backDepth, raceWorld);
        dofStatus={dofGate.reason,speedFocusTarget,0.0f,0.0f,0.0f};
        if (dofGate.runPass &&
            dofBuf1 != nullptr && backDepth != nullptr && DofPipeline() != nullptr) {
            // Smooth the speed in real time (about 0.8 s) so tunnel vision never pops.
            Uint64 nowTicks = SDL_GetTicksNS();
            float dtSeconds = speedFocusTicks ? (float)(nowTicks - speedFocusTicks) * 1e-9f : 0.0f;
            speedFocusTicks = nowTicks;
            if (dtSeconds > 0.25f) dtSeconds = 0.25f;
            speedFocusSmooth += (speedFocusTarget - speedFocusSmooth) * (1.0f - std::exp(-dtSeconds / 0.8f));
            ResolveProjection(&sceneNear, &sceneFar);
            float aspect = backHeight ? (float)backWidth / (float)backHeight : 1.0f;
            // x focus distance, y focus range (shallower with speed), z max blur, w tunnel strength 0..1
            // Optional (video.dof_focus_car, default 0): focus on the followed car instead of the fixed plane. Depth comes from the
            // car's own draws (world origin x view, forward axis), so it follows every camera; no car draw in front (cockpit) = fixed plane.
            float focusPos = dofFocalDistance / 100.0f, carDepth = 0.0f;
            if (Sys_GetOption("video.dof_focus_car", 0) != 0 && sceneNear > 0.0f && worldEnd < frame.commands.size() && frame.commands[worldEnd].playerObject != 0) {
                DofFocusAccum acc;
                for (const GfxCommand &oc : frame.commands)
                    if (oc.type == GFX_CMD_DRAW && oc.target == NULL && !oc.transformed) {
                        const GfxDrawState &os = frame.states[oc.state];
                        if (os.objectId == frame.commands[worldEnd].playerObject) { float p[3] = { os.world._41, os.world._42, os.world._43 }; acc.Add(&os.view._11, p); }
                    }
                float z, pos;
                if (acc.Depth(&z) && DofFocus_Position(z, sceneNear, sceneFar, &pos)) { focusPos = pos; carDepth = z; }
            }
            // Blur mode (video.blur_mode): 0 peripheral (default, the old look), 1 whole scene except the car. The car is located from its own
            // projected vertices (bounding rectangle + depth range), so this is an approximate region, not a per-pixel mask. No car (cockpit) = peripheral.
            int blurMode = Sys_GetOption("video.blur_mode", 0) != 0 ? 1 : 0, blurShown = 0;
            CarRegion carRegion;
            if (blurMode == 1 && sceneNear > 0.0f && worldEnd < frame.commands.size() && frame.commands[worldEnd].playerObject != 0) {
                for (const GfxCommand &oc : frame.commands)
                    if (oc.type == GFX_CMD_DRAW && oc.target == NULL && !oc.transformed) {
                        const GfxDrawState &os = frame.states[oc.state];
                        if (os.objectId != frame.commands[worldEnd].playerObject) continue;
                        const GfxVertex *vs = NULL;
                        if (oc.buffer != NULL) {
                            size_t first = oc.firstVertexByte / sizeof(GfxVertex);
                            if (first + oc.vertexCount <= oc.buffer->vertices.size()) vs = oc.buffer->vertices.data() + first;
                        } else if ((size_t)oc.firstVertexByte + (size_t)oc.vertexCount * sizeof(GfxVertex) <= frame.vertices.size())
                            vs = (const GfxVertex *)(frame.vertices.data() + oc.firstVertexByte);
                        if (vs == NULL) continue;
                        GfxViewport cv = ToTarget(os.viewport, os.screenW, os.screenH, true, backWidth, backHeight);
                        CarRegion_AddDraw(carRegion, &os.world._11, &os.view._11, &os.projection._11, (const unsigned char *)vs, (unsigned)sizeof(GfxVertex), oc.vertexCount,
                                          (float)cv.x, (float)cv.y, (float)cv.width, (float)cv.height, (float)backWidth, (float)backHeight);
                    }
            }
            float carPosMin = 0.0f, carPosMax = 0.0f;
            if (blurMode == 1 && carRegion.Valid() && DofFocus_Position(carRegion.zMin, sceneNear, sceneFar, &carPosMin) && DofFocus_Position(carRegion.zMax, sceneNear, sceneFar, &carPosMax)) blurShown = 1;
            else blurMode = 0;
            float uniforms[16] = { focusPos,
                                  dofFocalRange / 100.0f * (1.0f - 0.5f * std::min(speedFocusSmooth, 1.0f)),
                                  !dofGate.depthBlur ? 0.0f : dofMaxBlur / 100.0f * (float)(Sys_GetOption("video.dof_blur_max_px", 4) < 1 ? 1 : Sys_GetOption("video.dof_blur_max_px", 4) > 40 ? 40 : Sys_GetOption("video.dof_blur_max_px", 4)), speedFocusSmooth,
                                  aspect, kTunnelRadiusPerHeight * (float)backHeight, sceneNear, sceneFar,
                                  carRegion.u0, carRegion.v0, carRegion.u1, carRegion.v1,
                                  (float)blurShown, carPosMin, carPosMax, 0.02f };
            if (!blurShown) for (int k = 8; k < 16; ++k) uniforms[k] = 0.0f;
            if (!blurShown) uniforms[8] = Gfx_GetTunnelNearClear();   // peripheral mode: cockpit interior depth clear (see dof.frag)
            static int blurLogCounter = 0;
            if (Sys_GetOption("video.blur_mode", 0) != 0 && (blurLogCounter++ % 120) == 0)
                Sys_Log("blur mode: whole scene except car requested, %s; car rect u %.3f..%.3f v %.3f..%.3f (0..1, v downwards), view depth %.2f..%.2f world units, %d vertices; native pixels unverified",
                        blurShown ? "region found" : "NO car region, peripheral used", carRegion.u0, carRegion.u1, carRegion.v0, carRegion.v1, carRegion.zMin, carRegion.zMax, carRegion.count);
            SDL_GPUTextureSamplerBinding bindings[2] = {
                { presentSource, linearSampler }, { backDepth, depthSampler }
            };
            if (PostProcessPass(cmd, DofPipeline(), dofBuf1, bindings, 2,
                                uniforms, sizeof(uniforms)))
                {presentSource = dofBuf1;encoded|=8;
                 dofStatus={5,speedFocusTarget,speedFocusSmooth,uniforms[1],uniforms[2],focusPos,carDepth,Sys_GetOption("video.blur_mode", 0) != 0 ? (blurShown ? 1 : -1) : 0,carRegion.u0,carRegion.v0,carRegion.u1,carRegion.v1};}
        }
        if (godRaysEnabled && sunVisible && sampledDepth && godRaysIntensity > 0 &&
            godRaysBuf1 != nullptr && backDepth != nullptr && GodRaysPipeline() != nullptr) {
            float uniforms[4] = { godRaysIntensity / 255.0f, sunX, sunY, 0.0f };
            SDL_GPUTextureSamplerBinding bindings[3] = {
                { presentSource, linearSampler }, { backDepth, depthSampler },{cloudMask,linearSampler}
            };
            if (PostProcessPass(cmd, GodRaysPipeline(), godRaysBuf1, bindings, 3,
                                uniforms, sizeof(uniforms)))
                {presentSource = godRaysBuf1;encoded|=16;}
        }
        // Speed streak: directional radial streak of the scene (not a temporal blend). Reads presentSource, writes gradingTarget.
        {
            Uint64 nowTicks = SDL_GetTicksNS();
            float dtSeconds = speedStreakTicks ? (float)(nowTicks - speedStreakTicks) * 1e-9f : 0.0f;
            speedStreakTicks = nowTicks;
            if (dtSeconds > 0.25f) dtSeconds = 0.25f;
            speedStreakClock = std::fmod(speedStreakClock + dtSeconds, 1000.0f);
            speedStreakSmooth += (speedStreakTarget - speedStreakSmooth) * (1.0f - std::exp(-dtSeconds / 0.4f));
            if (!raceWorld) speedStreakSmooth = 0.0f;
        }
        // Vapor trails (video.vapor_trails, set by the game through SetVaporTrail): history of the car's rear anchors, projected for the shader.
        float vaporUniforms[60 * 4 + 4] = {};   // 4 trail boxes + 28 points + 28 depths + depth info, see speedstreak.frag
        float vaporNear = 0.0f, vaporFar = 0.0f;
        bool vaporDraw = false;
        {
            Uint64 vTicks = SDL_GetTicksNS();
            float rawDt = vaporTicks ? (float)(vTicks - vaporTicks) * 1e-9f : 0.0f;
            vaporTicks = vTicks;
            vaporSmooth += (vaporTarget - vaporSmooth) * (1.0f - std::exp(-std::min(rawDt, 0.25f) / 0.4f));
            if (!raceWorld || vaporTarget <= 0.0f) { if (vaporSmooth < 0.01f || !raceWorld) vaporSmooth = 0.0f; }
            float trailStrength = SpeedStreak_Smoothstep(0.30f, 0.90f, vaporSmooth);
            if (vaporSmooth <= 0.01f || !raceWorld || worldEnd >= frame.commands.size() || frame.commands[worldEnd].playerObject == 0) { vaporHistory.Reset(); vaporHavePrev = vaporHaveDir = false; }
            else if (trailStrength > 0.0f && sampledDepth && backDepth != nullptr) {   // trails are hidden by nearer geometry, which needs the scene depth: no depth = no trails
                const unsigned player = frame.commands[worldEnd].playerObject;
                const GfxDrawState *ref = NULL;   // any of the car's draws: they share the view, projection and viewport
                auto carVertices = [&](auto &&fn) {
                    for (const GfxCommand &oc : frame.commands) {
                        if (oc.type != GFX_CMD_DRAW || oc.target != NULL || oc.transformed) continue;
                        const GfxDrawState &os = frame.states[oc.state];
                        if (os.objectId != player) continue;
                        const GfxVertex *vs = NULL;
                        if (oc.buffer != NULL) {
                            size_t first = oc.firstVertexByte / sizeof(GfxVertex);
                            if (first + oc.vertexCount <= oc.buffer->vertices.size()) vs = oc.buffer->vertices.data() + first;
                        } else if ((size_t)oc.firstVertexByte + (size_t)oc.vertexCount * sizeof(GfxVertex) <= frame.vertices.size())
                            vs = (const GfxVertex *)(frame.vertices.data() + oc.firstVertexByte);
                        if (vs == NULL) continue;
                        if (ref == NULL) ref = &os;
                        for (uint32_t k = 0; k < oc.vertexCount; ++k) {
                            float lp[3] = { vs[k].x, vs[k].y, vs[k].z }, wp[3];
                            Vapor_ToWorld(&os.world._11, lp, wp);
                            fn(wp);
                        }
                    }
                };
                VaporBounds vb;
                carVertices([&](const float *wp) { vb.Add(wp); });
                ResolveProjection(&vaporNear, &vaporFar);
                if (vb.Valid() && ref != NULL && vaporNear > 0.0f && vaporFar > vaporNear) {
                    float centre[3]; vb.Centre(centre);
                    float carHeight = vb.hi[1] - vb.lo[1];
                    float jump = 0.0f;
                    if (vaporHavePrev) { float dx = centre[0] - vaporPrevCentre[0], dz = centre[2] - vaporPrevCentre[2]; jump = std::sqrt(dx * dx + dz * dz); }
                    if (vaporHavePrev && jump > 20.0f * carHeight) { vaporHistory.Reset(); vaporHaveDir = false; }   // teleport / restart: never draw a line across it
                    if (vaporHavePrev) { float d[3]; if (Vapor_Direction(vaporPrevCentre, centre, 0.01f * carHeight, d)) { for (int c = 0; c < 3; ++c) vaporDir[c] = d[c]; vaporHaveDir = true; } }
                    for (int c = 0; c < 3; ++c) vaporPrevCentre[c] = centre[c];
                    vaporHavePrev = true;
                    if (vaporHaveDir) {
                        VaporAnchors anchors;
                        carVertices([&](const float *wp) { Vapor_ConsiderAnchor(anchors, wp, centre, vaporDir, vb); });
                        vaporHistory.Step(rawDt, 0.25f, anchors);
                        GfxViewport cv = ToTarget(ref->viewport, ref->screenW, ref->screenH, true, backWidth, backHeight);
                        for (int k = 0; k < kVaporAnchors; ++k) {
                            float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f;
                            for (int j = 0; j < kVaporPoints; ++j) {
                                float *pt = &vaporUniforms[16 + (k * kVaporPoints + j) * 4];
                                float wp[3], u, v, z, sv;
                                int age = kVaporPickIndex[j];
                                if (!vaporHistory.Get(age, k, wp) || !Vapor_Project(&ref->view._11, &ref->projection._11, wp, (float)cv.x, (float)cv.y, (float)cv.width, (float)cv.height, (float)backWidth, (float)backHeight, &u, &v, &z, &sv)) continue;
                                float hw = Vapor_HalfWidth(carHeight, sv, age);
                                if (!(hw > 0.0f)) continue;
                                pt[0] = u; pt[1] = v; pt[2] = hw; pt[3] = Vapor_Alpha(trailStrength, age);
                                vaporUniforms[(4 + 28 + k * kVaporPoints + j) * 4] = z;
                                x0 = std::min(x0, u - 2.0f * hw); x1 = std::max(x1, u + 2.0f * hw); y0 = std::min(y0, v - 2.0f * hw); y1 = std::max(y1, v + 2.0f * hw);
                                vaporDraw = true;
                                vaporUniforms[60 * 4] = vaporNear; vaporUniforms[60 * 4 + 1] = vaporFar; vaporUniforms[60 * 4 + 2] = 0.5f * carHeight; vaporUniforms[60 * 4 + 3] = 0.5f * carHeight;
                            }
                            if (x1 > x0) { float *bx = &vaporUniforms[k * 4]; bx[0] = x0; bx[1] = y0; bx[2] = x1; bx[3] = y1; }
                        }
                    }
                }
            }
        }
        bool streakApplied = false;
        bool tunnelLinesOn = Sys_GetOption("video.tunnel_lines", 1) != 0 && speedFocusSmooth > 0.01f;   // radial lines inside the tunnel vision zone
        if ((speedStreakSmooth > 0.01f || vaporDraw || tunnelLinesOn) && raceWorld && gradingTarget != nullptr && presentSource != nullptr &&
            presentSource != gradingTarget && SpeedStreakPipeline() != nullptr) {
            float uniforms[4 + 60 * 4 + 4 + 4 + 4] = {};   // cockpitDepth and tunnelLines, after depthInfo
            uniforms[0] = speedStreakSmooth; uniforms[1] = backHeight ? (float)backWidth / (float)backHeight : 1.0f; uniforms[2] = speedStreakClock;
            uniforms[3] = Sys_GetOption("video.speed_lines", 1) != 0 ? speedStreakSmooth : 0.0f;   // radial lines have their own toggle
            memcpy(uniforms + 4, vaporUniforms, sizeof(vaporUniforms));
            float cockpitNear = 0.0f, cockpitFar = 0.0f;
            const float cockpitClear = Gfx_GetTunnelNearClear();
            if (cockpitClear > 0.0f && sampledDepth && backDepth != nullptr)
                ResolveProjection(&cockpitNear, &cockpitFar);
            const bool cockpitDepth = cockpitClear > 0.0f && cockpitNear > 0.0f && cockpitFar > cockpitNear;
            uniforms[4 + 60 * 4 + 4] = cockpitDepth ? cockpitClear : 0.0f;
            uniforms[4 + 60 * 4 + 5] = cockpitNear;
            uniforms[4 + 60 * 4 + 6] = cockpitFar;
            uniforms[4 + 60 * 4 + 8] = speedFocusSmooth; uniforms[4 + 60 * 4 + 9] = tunnelLinesOn ? 1.0f : 0.0f;
            SDL_GPUTextureSamplerBinding binding[2] = { { presentSource, linearSampler }, (vaporDraw || cockpitDepth) ? SDL_GPUTextureSamplerBinding{ backDepth, depthSampler } : SDL_GPUTextureSamplerBinding{ presentSource, linearSampler } };   // No trails or cockpit depth mask: the second sampler just needs some texture
            if (PostProcessPass(cmd, SpeedStreakPipeline(), gradingTarget, binding, 2, uniforms, sizeof(uniforms)))
                {presentSource = gradingTarget;encoded|=256;streakApplied = true;}
        }
        if ((colorGradingTemperature != 0 || colorGradingTint != 0 ||
             colorGradingSaturation != 100.0f || colorGradingContrast != 100.0f) &&
            gradingTarget != nullptr && godRaysBuf1 != nullptr && ColorGradingPipeline() != nullptr) {
            float uniforms[4] = { colorGradingTemperature / 100.0f, colorGradingTint / 100.0f,
                colorGradingSaturation / 100.0f, colorGradingContrast / 100.0f };
            SDL_GPUTextureSamplerBinding binding = { presentSource, linearSampler };
            SDL_GPUTexture *gradeOut = streakApplied ? godRaysBuf1 : gradingTarget;
            if (PostProcessPass(cmd, ColorGradingPipeline(), gradeOut, &binding, 1,
                                uniforms, sizeof(uniforms)))
                {presentSource = gradeOut;encoded|=32;}
        }

        { // Restrained sun response, after world effects, before HUD. Off by default.
            Uint64 now=SDL_GetTicksNS();float dt=sunGlareTicks?(now-sunGlareTicks)*1e-9f:0.f;sunGlareTicks=now;
            int amount=Sys_GetOption("video.sun_glare",0);
            float strength=sunGlare.Step(sunX,sunY,raceWorld&&sunVisible,amount,dt);
            if(raceWorld && amount>0 && strength>.0001f && sampledDepth && backDepth){
                if(!fsSunGlare)fsSunGlare=Shader(SHADER_CODE(sun_glare_frag),SDL_GPU_SHADERSTAGE_FRAGMENT,3,1);
                if(!sunGlarePipeline&&fsSunGlare)sunGlarePipeline=PostProcessPipeline(vsMotionBlur,fsSunGlare);
                float u[4]={sunX,sunY,strength,backHeight?(float)backWidth/backHeight:1.f};
                SDL_GPUTexture *target=presentSource==gradingTarget?godRaysBuf1:gradingTarget;
                SDL_GPUTextureSamplerBinding bindings[3]={{presentSource,linearSampler},{backDepth,depthSampler},{cloudMask,linearSampler}};
                if(sunGlarePipeline&&target&&PostProcessPass(cmd,sunGlarePipeline,target,bindings,3,u,sizeof(u)))presentSource=target;
            }
        }

        } else motionBlurHasPrevious=false;
        if(!raceWorld)motionBlurHasPrevious=false;
        if(worldEffects) {
            if(presentSource!=backColor){
                SDL_GPUBlitInfo blit={};blit.source.texture=presentSource;blit.source.w=backWidth;blit.source.h=backHeight;
                blit.destination.texture=backColor;blit.destination.w=backWidth;blit.destination.h=backHeight;
                blit.load_op=SDL_GPU_LOADOP_DONT_CARE;blit.filter=SDL_GPU_FILTER_NEAREST;SDL_BlitGPUTexture(cmd,&blit);
            }
            presentSource=backColor;
            bound=PassBindings();vertexUniformState=nullptr;fragmentUniformsSet=false;
            drawCommands(worldEnd+1,frame.commands.size());
        }
        // Read back before the frame is shown (the lens flare test).
        CollectReadback(readback);
        Readback *reading = NULL;
        if (readback.requested && readback.width > 0 && readback.height > 0) {
            int x = std::max(0, std::min(readback.x, backWidth - 1));
            int y = std::max(0, std::min(readback.y, backHeight - 1));
            int w = std::min(readback.width, backWidth - x), h = std::min(readback.height, backHeight - y);
            reading = FreeReadback(readback);
            if ((uint32_t)(w * h * 4) > reading->capacity) {
                SDL_ReleaseGPUTransferBuffer(device, reading->buffer);
                reading->capacity = (uint32_t)(w * h * 4);
                SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, reading->capacity, 0 };
                reading->buffer = SDL_CreateGPUTransferBuffer(device, &tbi);
            }
            SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
            SDL_GPUTextureRegion src = {};
            src.texture = backColor;
            src.x = x;
            src.y = y;
            src.w = w;
            src.h = h;
            src.d = 1;
            SDL_GPUTextureTransferInfo dst = { reading->buffer, 0, 0, 0 };
            SDL_DownloadFromGPUTexture(copy, &src, &dst);
            SDL_EndGPUCopyPass(copy);
            reading->width = w;
            reading->height = h;
            readback.requested = false;
        }


        if (vsync != presentModeVsync) {
            SDL_GPUPresentMode mode = vsync ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE;
            if (!vsync && !SDL_WindowSupportsGPUPresentMode(device, window, mode))
                mode = SDL_GPU_PRESENTMODE_MAILBOX;
            if (SDL_WindowSupportsGPUPresentMode(device, window, mode))
                SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode);
            presentModeVsync = vsync;
        }

        SDL_GPUTexture *swapchain = NULL;
        Uint32 sw = 0, sh = 0;
        double acquireStart = Sys_GetTicksPrecise();
        bool acquired = SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain, &sw, &sh);
        Gfx_NoteSwapchainWait(Sys_GetTicksPrecise() - acquireStart);
        if (acquired && swapchain != NULL) {
            if (sw != presentWidth || sh != presentHeight) {
                Sys_Log("gfx: %dx%d shown in %ux%u pixels", backWidth, backHeight, sw, sh);
                presentWidth = sw;
                presentHeight = sh;
            }
            // Movie frames fill the drawable; ordinary game frames retain their aspect ratio.
            PresentationRect rect = GetPresentationRect(backWidth, backHeight, sw, sh, stretch);
            SDL_GPUBlitInfo blit = {};
            blit.source.texture = presentSource;
            blit.source.w = backWidth;
            blit.source.h = backHeight;
            blit.destination.texture = swapchain;
            blit.destination.x = rect.x;
            blit.destination.y = rect.y;
            blit.destination.w = rect.width;
            blit.destination.h = rect.height;
            blit.load_op = SDL_GPU_LOADOP_CLEAR;
            blit.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
            blit.filter = smoothScaling ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
            bool scaled = false;
            if (metalFXSpatial && metalFXBlit != nullptr && !stretch &&
                filmGrain == 0 && chromaticAberration == 0)
                scaled = metalFXBlit(cmd, &blit);
            if (!scaled)
                SDL_BlitGPUTexture(cmd, &blit);
            if(metalFXSpatial)metalFXStatus=scaled?1:0;
            if (metalFXSpatial && !metalFXLogged) {
                Sys_Log("gfx: MetalFX spatial %s (%dx%d -> %dx%d); no FPS claim",
                    scaled ? "encoded" : "fallback", backWidth, backHeight, rect.width, rect.height);
                metalFXLogged = true;
            }
        }

        static unsigned lastEncoded=~0u;
        if(encoded!=lastEncoded){
            Sys_Log("effects: SDL_GPU passes encoded this frame (GPU completion and pixels unverified): AA=%s bloom=%s motion-blur=%s depth-blur=%s sun-rays=%s grade=%s shadow-pass=%s fog=%s",
                encoded&1?"yes":spatialAA?"fallback":"off",encoded&2?"yes":bloomEnabled?"fallback":"off",
                encoded&4?"yes":motionBlurEnabled?"seed/skipped/fallback":"off",encoded&8?"yes":dofEnabled?"fallback":"off",
                encoded&16?"yes":godRaysEnabled?sunVisible?"fallback":"sun-offscreen/skipped":"off",
                encoded&32?"yes":colorGradingTemperature||colorGradingTint||colorGradingSaturation!=100||colorGradingContrast!=100?"fallback":"off/neutral",encoded&64?"yes":shadowType?"fallback":"off",
                encoded&512?"yes":fogEnabled?sampledDepth?"fallback":"no-depth":"off");
            lastEncoded=encoded;
        }
        bool submitted;
        if (reading != nullptr) {
            reading->fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
            reading->serial = ++readbackSerial;
            submitted = reading->fence != nullptr;
        } else submitted = SDL_SubmitGPUCommandBuffer(cmd);
        static int lastSubmission=-1;
        if(lastSubmission!=(int)submitted){
            Sys_Log("effects: SDL_GPU command submission=%s (accepted is not completed/pixel verification)%s",submitted?"accepted":"FAILED",submitted?"":SDL_GetError());
            lastSubmission=submitted;
        }
        Gfx_SetRaysStatus(!godRaysEnabled?GFX_RAYS_OFF:encoded&16?GFX_RAYS_ENCODED:
            !worldEffects?GFX_RAYS_NO_WORLD:!sunVisible?GFX_RAYS_NO_SUN:
            !sampledDepth||!backDepth?GFX_RAYS_NO_DEPTH:godRaysIntensity<=0?GFX_RAYS_ZERO_AMOUNT:GFX_RAYS_RESOURCE);
        Gfx_SetDofStatus(dofStatus);
        Gfx_SetEffectStatus(requested,encoded,submitted?1:0,metalFXStatus);
        HashFrame();
    }

    // Takes the reads the GPU has finished, oldest first, so the newest
    // stays. Frame hashing waits for them all: its frames must not depend on
    // the GPU's timing.
    void CollectReadback(GfxReadback &readback) override
    {
        bool wait = Gfx_FrameHashWanted();
        for (;;) {
            Readback *oldest = NULL;
            for (Readback &r : readbacks)
                if (r.fence != NULL && (oldest == NULL || r.serial < oldest->serial))
                    oldest = &r;
            if (oldest == NULL || (!wait && !SDL_QueryGPUFence(device, oldest->fence)))
                return;
            FinishReadback(*oldest, readback);
        }
    }

    void FinishReadback(Readback &r, GfxReadback &readback)
    {
        SDL_WaitForGPUFences(device, true, &r.fence, 1);
        SDL_ReleaseGPUFence(device, r.fence);
        r.fence = NULL;
        const uint32_t *map = (const uint32_t *)SDL_MapGPUTransferBuffer(device, r.buffer, false);
        readback.pixels.assign(map, map + (size_t)r.width * r.height);
        SDL_UnmapGPUTransferBuffer(device, r.buffer);
        readback.readyWidth = r.width;
        readback.readyHeight = r.height;
        readback.ready = true;
    }

    // A slot for a new read. With all of them in flight, the GPU is far
    // behind: the oldest is finished first.
    Readback *FreeReadback(GfxReadback &readback)
    {
        Readback *oldest = NULL;
        for (Readback &r : readbacks) {
            if (r.fence == NULL)
                return &r;
            if (oldest == NULL || r.serial < oldest->serial)
                oldest = &r;
        }
        FinishReadback(*oldest, readback);
        return oldest;
    }

    // The back buffer's pixels for Gfx_HashFrame.
    void HashFrame()
    {
        if (!Gfx_FrameHashWanted())
            return;
        uint32_t bytes = (uint32_t)backWidth * backHeight * 4;
        SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, bytes, 0 };
        SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTextureRegion src = {};
        src.texture = backColor;
        src.w = backWidth;
        src.h = backHeight;
        src.d = 1;
        SDL_GPUTextureTransferInfo dst = { tb, 0, 0, 0 };
        SDL_DownloadFromGPUTexture(copy, &src, &dst);
        SDL_EndGPUCopyPass(copy);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        SDL_WaitForGPUFences(device, true, &fence, 1);
        SDL_ReleaseGPUFence(device, fence);
        Gfx_HashFrame((const uint8_t *)SDL_MapGPUTransferBuffer(device, tb, false), backWidth, backHeight);
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_ReleaseGPUTransferBuffer(device, tb);
    }
};

} // namespace

GfxExecutor *Gfx_CreateGpuExecutor(void)
{
    return new Executor;
}

bool Gfx_GpuIsAvailable(void)
{
    SDL_PropertiesID props = DeviceProperties();
    // Prefer a real Vulkan device (hardware-accelerated only); if none,
    // fall back to any GPU backend (Metal on macOS, Direct3D 12 on Windows,
    // etc.) so bloom, motion blur, particles and shadows are available there too.
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_REQUIRE_HARDWARE_ACCELERATION_BOOLEAN, true);
    if (SDL_GPUSupportsProperties(props)) {
        SDL_DestroyProperties(props);
        return true;
    }
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_REQUIRE_HARDWARE_ACCELERATION_BOOLEAN, false);
    bool available = SDL_GPUSupportsProperties(props);
    SDL_DestroyProperties(props);
    return available;
}
