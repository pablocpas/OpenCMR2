/*
 * OpenCMR2 renderer interface, implemented in src/render on SDL_GPU.
 *
 * It exposes the rendering model CMR2 was written for (Direct3D 7's
 * fixed-function pipeline) as immediate-mode calls on our own types: state
 * setters, then draws that use the current state. Draws are recorded with a
 * copy of their vertices and indices, so the game may overwrite its buffers
 * right after drawing, as it could with Direct3D.
 *
 * Enumerated values equal the original Direct3D constants, because the game
 * keeps them in its data and passes them around as plain numbers.
 */
#ifndef OPENCMR2_PORT_GFX_H
#define OPENCMR2_PORT_GFX_H

#include "port/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- types ---------------------------------------------------------------- */

typedef struct GfxTexture GfxTexture;
typedef struct GfxVertexBuffer GfxVertexBuffer;
/* World-space dust. TRUE accepts a bounded, preflighted volume command;
   caller suppresses cloud sheets, retaining ordinary chips/spray. FALSE uses fallback. */
typedef struct GfxDustLobe { float centreRadius[4]; float colourDensity[4]; } GfxDustLobe;
// Copy lobes for this view; true selects volume instead of fallback sheets.
BOOL Gfx_DrawDustVolume(const GfxDustLobe *lobes,int count);
// Encode pending world dust after opaque world/depth, before HUD.
void Gfx_FlushDustVolume(void);
// Diagnostic only (log lines, no rendering effect): camera position (world units) and forward vector used when the dust lobes were sampled.
void Gfx_NoteDustSamplingCamera(float x,float y,float z,float fx,float fy,float fz);
// Mark the completed world before queued HUD/menu layers are drawn.
void Gfx_EndWorldEffects(void);
// Explicit race draw scope; menu/background scenes must never enable weather.
void Gfx_SetRaceWorld(BOOL enabled);
void Gfx_SetBodyDirt(float amount, float red, float green, float blue);
// Stable id of the dynamic object (car slot + 1) for the draws that follow; 0 = static.
// Only temporal AA reads this, to learn which draws move.
void Gfx_SetObjectId(unsigned id);
/* Debug object mask: objectId (car slot + 1) of the car the current view follows; recorded with the world-end marker. */
void Gfx_SetPlayerObject(unsigned id);
void Gfx_SetCloudEnvironment(float coverage, float baseHeight, float sunX, float sunY, float sunZ);


/* Row-vector matrix, Direct3D layout: v' = v * M, translation in _41.._43. */
typedef struct GfxMatrix {
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
} GfxMatrix;

typedef struct GfxVector {
    float x, y, z;
} GfxVector;

typedef struct GfxColorValue {
    float r, g, b, a;
} GfxColorValue;

/* Colours are 0xAARRGGBB. */
#define GFX_RGBA(r, g, b, a) \
    ((DWORD)((((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)))

/* 3D vertex: position, normal, diffuse, specular, two texture coordinate
   sets (Direct3D FVF 0x2d2, 48 bytes). Lit and transformed by the device. */
typedef struct GfxVertex {
    float x, y, z;
    float nx, ny, nz;
    DWORD diffuse;
    DWORD specular;
    float u0, v0;
    float u1, v1;
} GfxVertex;

/* Pre-transformed 2D vertex: screen position, depth, 1/w, colours, one
   texture coordinate set (D3DTLVERTEX, 32 bytes). */
typedef struct GfxTLVertex {
    float sx, sy, sz, rhw;
    DWORD color;
    DWORD specular;
    float tu, tv;
} GfxTLVertex;

enum GfxLightType {
    GFX_LIGHT_POINT = 1,
    GFX_LIGHT_SPOT = 2,
    GFX_LIGHT_DIRECTIONAL = 3,
    GFX_LIGHT_PARALLELPOINT = 4
};

typedef struct GfxLight {
    int type;
    GfxColorValue diffuse;
    GfxColorValue specular;
    GfxColorValue ambient;
    GfxVector position;
    GfxVector direction;
    float range;
    float falloff;
    float attenuation0, attenuation1, attenuation2;
    float theta, phi;
} GfxLight;

typedef struct GfxMaterial {
    GfxColorValue diffuse;
    GfxColorValue ambient;
    GfxColorValue specular;
    GfxColorValue emissive;
    float power;
} GfxMaterial;

typedef struct GfxViewport {
    DWORD x, y, width, height;
    float minZ, maxZ;
} GfxViewport;

enum GfxPrimitive {
    GFX_POINTLIST = 1,
    GFX_LINELIST = 2,
    GFX_LINESTRIP = 3,
    GFX_TRIANGLELIST = 4,
    GFX_TRIANGLESTRIP = 5,
    GFX_TRIANGLEFAN = 6
};

enum GfxTransformType {
    GFX_TRANSFORM_WORLD = 1,
    GFX_TRANSFORM_VIEW = 2,
    GFX_TRANSFORM_PROJECTION = 3,
    GFX_TRANSFORM_TEXTURE0 = 16,
    GFX_TRANSFORM_TEXTURE1 = 17
};

enum GfxCompare {
    GFX_CMP_NEVER = 1,
    GFX_CMP_LESS = 2,
    GFX_CMP_EQUAL = 3,
    GFX_CMP_LESSEQUAL = 4,
    GFX_CMP_GREATER = 5,
    GFX_CMP_NOTEQUAL = 6,
    GFX_CMP_GREATEREQUAL = 7,
    GFX_CMP_ALWAYS = 8
};

enum GfxBlend {
    GFX_BLEND_ZERO = 1,
    GFX_BLEND_ONE = 2,
    GFX_BLEND_SRCCOLOR = 3,
    GFX_BLEND_INVSRCCOLOR = 4,
    GFX_BLEND_SRCALPHA = 5,
    GFX_BLEND_INVSRCALPHA = 6,
    GFX_BLEND_DESTALPHA = 7,
    GFX_BLEND_INVDESTALPHA = 8,
    GFX_BLEND_DESTCOLOR = 9,
    GFX_BLEND_INVDESTCOLOR = 10,
    GFX_BLEND_SRCALPHASAT = 11
};

enum GfxCull {
    GFX_CULL_NONE = 1,
    GFX_CULL_CW = 2,
    GFX_CULL_CCW = 3
};

enum GfxFogMode {
    GFX_FOG_NONE = 0,
    GFX_FOG_EXP = 1,
    GFX_FOG_EXP2 = 2,
    GFX_FOG_LINEAR = 3
};

/* Where a material colour comes from when vertex colours are enabled. */
enum GfxMaterialSource {
    GFX_MCS_MATERIAL = 0,
    GFX_MCS_COLOR1 = 1,
    GFX_MCS_COLOR2 = 2
};

enum GfxTextureOp {
    GFX_TOP_DISABLE = 1,
    GFX_TOP_SELECTARG1 = 2,
    GFX_TOP_SELECTARG2 = 3,
    GFX_TOP_MODULATE = 4,
    GFX_TOP_MODULATE2X = 5,
    GFX_TOP_MODULATE4X = 6,
    GFX_TOP_ADD = 7,
    GFX_TOP_ADDSIGNED = 8,
    GFX_TOP_ADDSIGNED2X = 9,
    GFX_TOP_SUBTRACT = 10,
    GFX_TOP_BUMPENVMAP = 22,
    GFX_TOP_BUMPENVMAPLUMINANCE = 23
};

/* Texture stage arguments, optionally combined with the two modifiers. */
enum GfxTextureArg {
    GFX_TA_DIFFUSE = 0,
    GFX_TA_CURRENT = 1,
    GFX_TA_TEXTURE = 2,
    GFX_TA_TFACTOR = 3,
    GFX_TA_SPECULAR = 4,
    GFX_TA_COMPLEMENT = 0x10,
    GFX_TA_ALPHAREPLICATE = 0x20
};

enum GfxFilter {               /* minification and magnification */
    GFX_FILTER_POINT = 1,
    GFX_FILTER_LINEAR = 2
};

enum GfxMipFilter {
    GFX_MIPFILTER_NONE = 1,
    GFX_MIPFILTER_POINT = 2,
    GFX_MIPFILTER_LINEAR = 3
};

enum GfxAddress {
    GFX_ADDRESS_WRAP = 1,
    GFX_ADDRESS_MIRROR = 2,
    GFX_ADDRESS_CLAMP = 3
};

/* Texture coordinate index of a stage: the vertex set in the low bits,
   optionally generated instead. */
#define GFX_TCI_CAMERASPACEREFLECTIONVECTOR 0x30000

/* Texture transform: number of coordinates it outputs. */
enum GfxTextureTransform {
    GFX_TTFF_DISABLE = 0,
    GFX_TTFF_COUNT2 = 2,
    GFX_TTFF_COUNT3 = 3
};

/* ---- device and frame ------------------------------------------------------ */

/* The SDL window flags the renderer needs, set up before the window is made
   (SDL_WindowFlags; OpenGL's context is chosen with its window). */
unsigned long long Gfx_PrepareWindow(void);
/* The renderer's name for the graphics menu: "OpenCMR2 SDL_GPU" or
   "OpenCMR2 OpenGL". */
const char *Gfx_GetRendererName(void);
/* Creates the GPU device on first use and sets the back buffer size and
   window mode (it may be called again to change them). */
BOOL Gfx_SetVideoMode(int width, int height, BOOL fullscreen);
/* Switches the window between fullscreen and windowed, keeping the mode. */
void Gfx_SetFullscreen(BOOL fullscreen);
/* OpenCMR2: the anisotropic filtering level (1: off, as the original sampled;
    the higher, the sharper textures seen at a slant). Takes effect at once,
    without recreating the device. [video] video.anisotropy */
void Gfx_SetAnisotropy(int level);
/* OpenCMR2: motion blur between frames. `enabled` turns it on or off and
   `mix` (0..255) is how much of the previous frame to blend into the current
   one. Takes effect at once, between frames. [video] video.motion_blur */
void Gfx_SetMotionBlur(BOOL enabled, int mix);
/* OpenCMR2: bloom. `enabled` turns it on or off and `intensity` (0..255)
   scales how strongly the blurred bright pixels are added back to the frame.
   Takes effect at once, between frames. [video] video.bloom */
void Gfx_SetBloom(BOOL enabled, int intensity);
/* OpenCMR2: particles (rain/snow). `type` 0=off, 1=rain, 2=snow; `intensity`
   (0..255) scales the emission rate. Takes effect at once. [video]
   video.particles */
void Gfx_SetParticles(int type, int intensity);
/* OpenCMR2: directional shadows. `type` 0=off, 1=on; `bias` (0..100) is the
   shadow depth bias. Takes effect at once. [video] video.shadows */
void Gfx_SetShadows(int type, int bias);
/* OpenCMR2: lens grime post-process effects, applied after the scene is
   rendered. `video.chromatic_aberration` (0..100) is color-fringing strength
   toward screen edges. `video.vignette` (0..100) darkens corners and edges.
   `video.film_grain` (0..100) adds a subtle noise overlay. SDL_GPU only. */
void Gfx_SetChromaticAberration(int strength);
void Gfx_SetVignette(int strength);
void Gfx_SetFilmGrain(int strength);
/* OpenCMR2: depth of field. `enabled` turns it on/off, `focalDistance`
   (0..100) is where the focus plane sits in depth, `focalRange` (0..100)
   is how wide the in-focus band is, `maxBlur` (0..100) is the blur radius.
   SDL_GPU only. */
void Gfx_SetDepthOfField(BOOL enabled, int focalDistance, int focalRange, int maxBlur);
/* OpenCMR2: speed-driven tunnel vision for the depth of field pass. `speed` is 0..1 (0 = standing,
   1 = fast). With depth_of_field on, the sharp zone narrows toward the middle of the screen and the
   focus band gets shallower as it rises. Purely visual. SDL_GPU only. */
void Gfx_SetSpeedFocus(float speed);
// Render-view gate: clears residual tunnel/streak smoothing when a drone is drawn.
void Gfx_SetSpeedEffectsSuppressed(BOOL suppressed);
/* OpenCMR2: drop the eased tunnel vision and speed streak at once (targets and eased values to 0), for views where they must not show, e.g. external replay cameras. */
void Gfx_ClearSpeedEffects(void);
/* OpenCMR2: drop the vapor trail (target, eased value and stored trail points) at once, e.g. during replay playback. */
void Gfx_ClearVaporTrail(void);
/* OpenCMR2 cockpit view: pixels nearer than this view distance (world units, the interior) stay out of the tunnel-vision blur. 0 = off. SDL_GPU only. */
void Gfx_SetTunnelNearClear(float worldUnits);
/* OpenCMR2: tunnel road tracking (video.tunnel_track). u/v = the road-ahead point in screen uv (v downwards), valid 1 when the projection succeeded this frame; invalid eases the focus back to the centre. */
void Gfx_SetTunnelFocus(float u, float v, int valid);
float Gfx_GetTunnelNearClear(void);
/* OpenCMR2: speed streak strength 0..1 (visual only; directional radial streak, SDL_GPU backend). */
void Gfx_SetSpeedStreak(float strength);
/* OpenCMR2: vapor trail strength 0..1 (visual only; thin trails shed behind the followed car, SDL_GPU backend, 0 = off). */
void Gfx_SetVaporTrail(float strength);
/* OpenCMR2: god rays / volumetric lighting. `enabled` turns it on/off,
   `intensity` (0..255) scales the effect strength. SDL_GPU only. */
void Gfx_SetGodRays(BOOL enabled, int intensity);
// OpenCMR2: optional atmosphere fog. amount 0..200 (percent, 100 = heavy default),
// glow 0..100 (sun-ward brightening of the fog).
void Gfx_SetPostFog(BOOL enabled, int amount, int glow);
void Gfx_SetPostFogClearDistance(int worldUnits);
void Gfx_SetSunScreenPosition(float x, float y, BOOL visible);
// One car light projected to the screen for the fog pass (video.fog_lights).
struct GfxFogLight {
    float x, y;       // screen uv (v downwards)
    float depthRef;   // depth-buffer value half a world unit behind the lamp anchor (game projection) so own-bodywork occludes; <= 0 disables the depth gate
    float intensity;  // 0..1
    float r, g, b;    // tint 0..1 (headlight warm white, rear/indicator per the glow)
    float beamX, beamY; // aspect-corrected projected lamp-to-end unit vector; (0,0) = no projected run
    float beamLen;      // retained projected wedge length, capped to a screen fraction
    float haloX, haloY; // screen uv of the glow anchor, a few units forward along the beam (the lit air in front of the car)
    float haloRef;      // unbiased forward segment-anchor depth; legacy halo ref otherwise
    float beamFade;     // beam depth ratio q = anchor/end (near-plane-clamped); < 1 forward soft tail, > 1 backward energy thinning
    float beamEndRef;   // depth-buffer value at the beam's far end (same convention as depthRef); <= 0: no per-segment gate, fall back to the constant lamp gate
    float startRamp;    // aspect-corrected screen length over which the wedge fades in from the anchor; > 0 only for lamps at/behind the near plane (the anchor is then the beam's near-plane crossing, not a real lamp spot); 0 = no ramp
};
// Up to 8 car lights for the fog pass; count 0 clears. Called per view per frame.
void Gfx_SetFogLights(const GfxFogLight *lights, int count);
/* World-anchored fog banks (video.fog_spatial): camera horizontal position, forward and right axes (horizontal parts), 1/projection x scale and bank strength 0..1 (0 = uniform fog). Called per view per frame. */
void Gfx_SetFogCamera(float posX, float posZ, float fwdX, float fwdZ, float rightX, float rightZ, float invP11, float strength);
void Gfx_SetShadowQuality(int resolution, int softness);
/* OpenCMR2: color grading. `temperature` (-100..100) cool->warm,
   `tint` (-100..100) green->magenta, `saturation` (0..200) color strength,
   `contrast` (0..200) contrast around middle gray. SDL_GPU only. */
void Gfx_SetColorGrading(int temperature, int tint, int saturation, int contrast);
/* OpenCMR2: whether the OpenGL 3.3 renderer is active (SDL_GPU otherwise).
   Bloom, motion blur, particles and shadows are SDL_GPU-only. */
int Gfx_IsOpenGL(void);
void Gfx_Shutdown(void);

/* Resolutions to offer, smallest first. */
int Gfx_GetDisplayModeCount(void);
void Gfx_GetDisplayMode(int index, int *width, int *height);
/* The resolution of a new configuration, one of the modes offered: the
   display's own. */
void Gfx_GetDefaultDisplayMode(int *width, int *height);

/* A frame is BeginScene...EndScene blocks (any number) and one Present. */
void Gfx_BeginScene(void);
void Gfx_EndScene(void);
void Gfx_Present(BOOL vsync);
/* Movie presentation: stretch the back buffer to the entire drawable area. */
void Gfx_PresentStretched(BOOL vsync);

/* OpenCMR2: statistics of the frame last presented (draw calls, vertices,
   indices and triangles), for the in-game debug overlay. [video]
   video.debug_draw enables it. */
// Previous presented frame: encoded commands, not completed pixels.
typedef struct GfxEffectStatus {unsigned requested,encoded;int submitted;int metalFX;} GfxEffectStatus;
void Gfx_SetEffectStatus(unsigned requested,unsigned encoded,int submitted,int metalFX);
GfxEffectStatus Gfx_GetEffectStatus(void);
// Previous submitted frame rays gate, independent of the user's request.
enum {GFX_RAYS_UNKNOWN, GFX_RAYS_OFF, GFX_RAYS_ENCODED, GFX_RAYS_NO_WORLD,
      GFX_RAYS_NO_SUN, GFX_RAYS_NO_DEPTH, GFX_RAYS_ZERO_AMOUNT, GFX_RAYS_RESOURCE};
/* OpenCMR2: previous-frame depth of field / tunnel vision status for the debug overlay.
   reason: 0 unknown, 1 DoF off, 2 no sampled depth, 3 range or blur is zero, 4 pass resources
   unavailable, 5 encoded. `target` is the requested speed amount 0..1, `smooth` the amount actually
   sent to the shader, `range` the effective focus range 0..1, `blur` the effective blur radius.
   Encoded means submitted to the GPU, not proof of pixels. */
typedef struct GfxDofStatus {int reason;float target,smooth,range,blur;float focus,carDepth;int blurMode;float carU0,carV0,carU1,carV1;float trackU,trackV;int trackValid;} GfxDofStatus;   /* track* = the eased tunnel road-tracking focus (uv, v downwards); trackValid 0 = screen centre (tracking off or unavailable) */   /* blurMode 0 peripheral, 1 whole scene except the car (SDL_GPU only), -1 requested but the car region was unavailable (peripheral used); car* = the car rectangle (uv) */ /* carDepth > 0: focus follows the car (view-space depth); 0: fixed plane */
void Gfx_SetDofStatus(GfxDofStatus status);
/* Diagnostic only (cockpit fog investigation): how the last fog pass recovered view distance.
   mode: 0 no fog pass this frame, 1 single projection (true view distance), 2 ambiguous (>1 near/far pair,
   raw-depth ramp used), 3 no usable perspective projection (raw-depth ramp used). pairs = distinct near/far
   pairs voted by depth-writing draws; near/far = dominant pair; votes = draws behind that pair. */
typedef struct GfxFogDepthStatus {int mode,pairs,votes;float nearZ,farZ;} GfxFogDepthStatus;
void Gfx_SetFogDepthStatus(GfxFogDepthStatus status);
GfxFogDepthStatus Gfx_GetFogDepthStatus(void);
/* Fog depth probe (video.fog_probe, SDL_GPU only, opt-in one-shot): the verdict of the last
   probe readback - what depth the fog pass actually saw over a windscreen-centred region versus
   the fog factor that depth should have produced and the color shift the pass actually applied.
   state: 0 no probe run yet, 1 near depth across the region (residual pollution), 2 far depth but
   the density curve is weak there, 3 far depth and a strong expected factor but no output shift
   (pass/compositing failure), 4 output shift consistent with fog, 5 projection unknown (raw-depth
   ramp active, distances not recoverable). viewZ = median recovered view distance, expected =
   mean expected fog factor (banks/beams excluded), measured = mean before/after color shift 0..1. */
typedef struct GfxFogProbeStatus {int state; float viewZ, expected, measured;} GfxFogProbeStatus;
void Gfx_SetFogProbeStatus(GfxFogProbeStatus status);
GfxFogProbeStatus Gfx_GetFogProbeStatus(void);
GfxDofStatus Gfx_GetDofStatus(void);
/* Diagnostic read of the current draw state: out[0..9] = depthTest, depthWrite, depthFunc, alphaBlend, srcBlend, destBlend, alphaTest, alphaRef, fog, lighting. */
void Gfx_GetBlendDepthState(int *out);
void Gfx_SetRaysStatus(int reason);
int Gfx_GetRaysStatus(void);
DWORD Gfx_GetFrameDrawCalls(void);
/* OpenCMR2 diagnostics: ms inside the frame executor and, of that, the swapchain wait, summed since launch. */
void Gfx_NoteSwapchainWait(double ms);
void Gfx_GetPresentTimeTotals(double *execMs, double *waitMs);
/* OpenCMR2 diagnostics: number of Present() calls since launch (monotonic). */
unsigned long long Gfx_GetPresentCount(void);
DWORD Gfx_GetFrameVertices(void);
DWORD Gfx_GetFrameIndices(void);
DWORD Gfx_GetFrameTriangles(void);

enum GfxClearFlags {
    GFX_CLEAR_TARGET = 1,
    GFX_CLEAR_ZBUFFER = 2
};
void Gfx_Clear(DWORD flags, DWORD color, float z);
void Gfx_SetViewport(const GfxViewport *viewport);
void Gfx_GetViewport(GfxViewport *viewport);
/* The size of the screen the game draws the back buffer in: its viewports,
   clears and pre-transformed vertices are scaled from it to the back
   buffer's pixels, which are still rasterised at their resolution. 0, 0 is
   the back buffer's own size. Render-target textures are not scaled. */
void Gfx_SetVirtualScreen(int width, int height);

/* Preserve drawing state around temporary passes such as movie playback.
   Referenced textures must remain alive until the matching PopState. */
void Gfx_PushState(void);
void Gfx_PopState(void);

/* Draws into a render-target texture (or a cube face from Gfx_GetCubeFace)
   or, with NULL, the back buffer. */
void Gfx_SetRenderTarget(GfxTexture *target);

/* Distance fade: keeps a copy of the back buffer as it is now (the sky and
   the backdrop, drawn first), and 3D draws into the back buffer then blend
   into that copy between distances start and end from the eye, so the
   scenery dissolves into what lies behind it before the far plane cuts it. end <= start turns
   the fade off. */
void Gfx_CaptureBackdrop(void);
/* OpenCMR2: save the next presented frame (HUD and overlay included) as a PNG at `path`. */
void Gfx_RequestCapture(const char *path);
/* Number of capture requests the renderer has processed (saved or failed). */
unsigned Gfx_GetCaptureSerial(void);
void Gfx_SetDistanceFade(float start, float end);

/* ---- fixed-function state -------------------------------------------------- */

void Gfx_SetTransform(int type, const GfxMatrix *matrix);
void Gfx_GetTransform(int type, GfxMatrix *matrix);

void Gfx_SetLight(int index, const GfxLight *light);
void Gfx_GetLight(int index, GfxLight *light);
void Gfx_EnableLight(int index, BOOL enable);
void Gfx_SetMaterial(const GfxMaterial *material);

/* One setter per render state the game uses; the values are Direct3D's. */
void Gfx_SetLighting(BOOL enable);
void Gfx_SetAmbient(DWORD color);
void Gfx_SetSpecular(BOOL enable);
void Gfx_SetLocalViewer(BOOL enable);
void Gfx_SetNormalizeNormals(BOOL enable);
void Gfx_SetColorVertex(BOOL enable);
void Gfx_SetDiffuseMaterialSource(int source);
void Gfx_SetSpecularMaterialSource(int source);
void Gfx_SetAmbientMaterialSource(int source);
void Gfx_SetEmissiveMaterialSource(int source);

void Gfx_SetFog(BOOL enable);
void Gfx_SetFogColor(DWORD color);
void Gfx_SetFogVertexMode(int mode);
void Gfx_SetFogTableMode(int mode);
void Gfx_SetFogStart(float start);
void Gfx_SetFogEnd(float end);

void Gfx_SetDepthTest(BOOL enable);
void Gfx_SetDepthWrite(BOOL enable);
void Gfx_SetDepthFunc(int compare);
void Gfx_SetCullMode(int cull);
void Gfx_SetAlphaBlend(BOOL enable);
void Gfx_SetSrcBlend(int blend);
void Gfx_SetDestBlend(int blend);
void Gfx_SetAlphaTest(BOOL enable);
void Gfx_SetAlphaFunc(int compare);
void Gfx_SetAlphaRef(DWORD reference);
void Gfx_SetTextureFactor(DWORD color);

/* Texture stages 0 and 1; stage 2 is accepted and ignored (no draw uses
   more than two). */
void Gfx_SetTexture(int stage, GfxTexture *texture);
void Gfx_SetStageColorOp(int stage, int op);
void Gfx_SetStageColorArg1(int stage, int arg);
void Gfx_SetStageColorArg2(int stage, int arg);
void Gfx_SetStageAlphaOp(int stage, int op);
void Gfx_SetStageAlphaArg1(int stage, int arg);
void Gfx_SetStageAlphaArg2(int stage, int arg);
void Gfx_SetStageAddress(int stage, int address);
void Gfx_SetStageMinFilter(int stage, int filter);
void Gfx_SetStageMagFilter(int stage, int filter);
void Gfx_SetStageMipFilter(int stage, int filter);
void Gfx_SetStageMaxMipLevel(int stage, int level);
void Gfx_SetStageTexCoordIndex(int stage, DWORD index);
void Gfx_SetStageTextureTransform(int stage, int flags);
/* index: 0 = m00, 1 = m01, 2 = m10, 3 = m11 */
void Gfx_SetStageBumpEnvMat(int stage, int index, float value);
void Gfx_SetStageBumpEnvLScale(int stage, float value);
void Gfx_SetStageBumpEnvLOffset(int stage, float value);

/* ---- geometry ---------------------------------------------------------------- */

GfxVertexBuffer *Gfx_CreateVertexBuffer(DWORD vertexCount);
void Gfx_DestroyVertexBuffer(GfxVertexBuffer *buffer);
/* The whole buffer, for writing; contents are kept between locks. */
GfxVertex *Gfx_LockVertexBuffer(GfxVertexBuffer *buffer);
void Gfx_UnlockVertexBuffer(GfxVertexBuffer *buffer);

void Gfx_DrawPrimitiveVB(int primitive, GfxVertexBuffer *buffer, DWORD firstVertex, DWORD vertexCount);
/* Indices are relative to firstVertex. */
void Gfx_DrawIndexedPrimitiveVB(int primitive, GfxVertexBuffer *buffer, DWORD firstVertex, DWORD vertexCount,
                                const WORD *indices, DWORD indexCount);
void Gfx_DrawPrimitive(int primitive, const GfxVertex *vertices, DWORD vertexCount);
void Gfx_DrawIndexedPrimitive(int primitive, const GfxVertex *vertices, DWORD vertexCount, const WORD *indices,
                              DWORD indexCount);
void Gfx_DrawPrimitiveTL(int primitive, const GfxTLVertex *vertices, DWORD vertexCount);
void Gfx_DrawIndexedPrimitiveTL(int primitive, const GfxTLVertex *vertices, DWORD vertexCount, const WORD *indices,
                                DWORD indexCount);

/* ---- textures -------------------------------------------------------------- */

enum GfxFormat {
    GFX_FORMAT_BGRX8,         /* 32-bit, no alpha */
    GFX_FORMAT_BGRA8,         /* 32-bit with alpha */
    GFX_FORMAT_BC1,           /* DXT1 */
    GFX_FORMAT_BC3,           /* DXT5 */
    GFX_FORMAT_BUMP_DUDVL     /* 24-bit: signed dU, signed dV, luminance (L8V8U8) */
};

enum GfxPixelFormatFlags {
    GFX_PF_ALPHA = 0x1,     /* has an alpha channel */
    GFX_PF_FOURCC = 0x4,    /* compressed, see fourCC */
    GFX_PF_RGB = 0x40,
    GFX_PF_BUMP = 0x40000   /* dU/dV/luminance */
};

/* Layout of a format, for the game's pixel code (DDPIXELFORMAT's flags). */
typedef struct GfxPixelFormat {
    DWORD flags;
    DWORD bitsPerPixel;
    DWORD redMask, greenMask, blueMask, alphaMask;   /* bump: dU, dV, luminance masks */
    DWORD fourCC;                                     /* compressed formats: 'DXT1', 'DXT5' */
} GfxPixelFormat;
void Gfx_GetPixelFormat(int format, GfxPixelFormat *out);

enum GfxTextureFlags {
    GFX_TEXTURE_RENDER_TARGET = 1,    /* can be drawn into (with its own depth buffer) */
    GFX_TEXTURE_CUBE = 2,             /* six faces */
    GFX_TEXTURE_DYNAMIC = 4,          /* changed often */
    GFX_TEXTURE_STAGING = 8,          /* CPU only: a source for Gfx_CopyTexture */
    GFX_TEXTURE_DISTANCE_FIELD = 16,  /* alpha is a signed distance to the edge, 0.5 on it: drawn with
                                         an edge of one pixel at any scale */
    GFX_TEXTURE_MENU_DOT = 32,       /* the original large menu dot, drawn as a circle when enlarged */
    GFX_TEXTURE_VECTOR_ICON = 64     /* traced colour/alpha fields, with native source bound separately */
};

/* mipLevels counts the top level (1 = no mipmaps). */
GfxTexture *Gfx_CreateTexture(int width, int height, int format, int mipLevels, DWORD flags);
void Gfx_DestroyTexture(GfxTexture *texture);
int Gfx_GetTextureMipCount(GfxTexture *texture);

typedef struct GfxLockedRect {
    void *pixels;
    LONG pitch;               /* bytes per row (per row of 4x4 blocks if compressed) */
    DWORD width, height;      /* of the mip level */
    GfxPixelFormat format;
} GfxLockedRect;

/* Locks a mip level of a face for reading and writing; the texture is
   updated on the GPU when it is unlocked. */
BOOL Gfx_LockTexture(GfxTexture *texture, int face, int mip, GfxLockedRect *out);
void Gfx_UnlockTexture(GfxTexture *texture, int face, int mip);

/* Draws replacement wherever texture is drawn (NULL: texture itself), with
   the same texture coordinates; it is destroyed with texture. */
void Gfx_SetTextureReplacement(GfxTexture *texture, GfxTexture *replacement);
/* Texels of the distance field texture is drawn from (its replacement) per
   texel of its own, or 0 if it is drawn from its own pixels. */
int Gfx_GetDistanceFieldScale(GfxTexture *texture);
/* Give the large menu matrix dot a geometric replacement. Its original
   pixels are kept for native-size and smaller draws; the game still queues
   the same dots, colours and positions. */
void Gfx_VectorizeMenuDot(GfxTexture *texture);
/* Half a source texel for bitmaps, half a field texel for distance fields,
   and zero for geometry (which must not be cropped to texel centres). */
float Gfx_GetSpriteTexelInset(GfxTexture *texture);

/* Fills the lower mip levels from the top one (box filter). */
void Gfx_GenerateMipmaps(GfxTexture *texture);
/* Copies the top level of src into dst, converting the format (compressed
   sources are decoded) and stretching to dst's size like a DirectDraw Blt. */
void Gfx_CopyTexture(GfxTexture *dst, GfxTexture *src);

/* Cube maps: a texture for one face, usable as a render target. Face 0 is
   the cube itself; destroying a face does nothing. */
GfxTexture *Gfx_GetCubeFace(GfxTexture *cube, int face);

/* Asks for a copy of a rectangle of the back buffer at the end of this frame
   and returns, in pixels (width * height colours, 0xAARRGGBB, rows top to
   bottom), the copy taken at the end of the previous frame. Returns 0 when
   there is none yet for a rectangle of that size. */
BOOL Gfx_ReadBackBuffer(int x, int y, int width, int height, DWORD *pixels);

#ifdef __cplusplus
}
#endif

#endif
