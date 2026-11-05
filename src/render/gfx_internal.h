// Internals of the renderer: the state gfx_core.cpp records and the
// executor that turns a recorded frame into GPU work (gfx_gpu.cpp on
// SDL_GPU, gfx_gl.cpp on OpenGL 3.3, or the null one for headless runs).
#ifndef OPENCMR2_GFX_INTERNAL_H
#define OPENCMR2_GFX_INTERNAL_H

#include "port/gfx.h"

#include <stdint.h>

#include <vector>

// ---- textures and buffers --------------------------------------------------------

struct GfxTexture {
    int width, height;
    int format;                 // GFX_FORMAT_*
    int mipLevels;
    DWORD flags;
    GfxTexture *cube;           // a face view: the cube it belongs to
    int face;                   // a face view: its face
    GfxTexture *faceViews[6];   // a cube: its face views (1..5; 0 is itself)
    // CPU copy of every level of every face, tightly packed rows.
    std::vector<uint8_t> levels[6][16];
    bool dirty;                 // CPU copy newer than the GPU's
    bool renderedTo;            // the GPU copy is newer (render targets)
    void *gpu;                  // executor's object
    GfxTexture *replacement;    // drawn instead (Gfx_SetTextureReplacement)
};

// The texture a draw reads for t: its cube, or its replacement.
inline GfxTexture *Gfx_DrawnTexture(GfxTexture *t)
{
    if (t != nullptr && t->cube != nullptr)
        t = t->cube;
    return t != nullptr && t->replacement != nullptr ? t->replacement : t;
}

// A vertex buffer the game fills through Lock/Unlock. Draws from one that
// has not been locked for a while refer to its GPU copy (stage geometry);
// draws from one locked recently copy their vertices (cars, sprites), as the
// game rewrites those during the frame.
struct GfxVertexBuffer {
    std::vector<GfxVertex> vertices;
    uint32_t version = 1;           // bumped by every lock
    uint32_t lastLockFrame = 0;     // presented frame of the last lock
    uint32_t referencedRecord = 0;  // recording that last drew from it
    void *gpu = nullptr;            // executor's copy
};

// Bytes of one level, given its size, for a format.
size_t Gfx_LevelSize(int format, int width, int height, LONG *pitch);
// Bytes per pixel of the CPU copy (0 for compressed formats).
int Gfx_BytesPerPixel(int format);

// ---- recorded state ---------------------------------------------------------------

struct GfxStageState {
    int colorOp = GFX_TOP_MODULATE, colorArg1 = GFX_TA_TEXTURE, colorArg2 = GFX_TA_CURRENT;
    int alphaOp = GFX_TOP_SELECTARG1, alphaArg1 = GFX_TA_TEXTURE, alphaArg2 = GFX_TA_CURRENT;
    int address = GFX_ADDRESS_WRAP;
    int minFilter = GFX_FILTER_POINT, magFilter = GFX_FILTER_POINT, mipFilter = GFX_MIPFILTER_NONE;
    int maxMipLevel = 0;
    DWORD texCoordIndex = 0;
    int textureTransform = GFX_TTFF_DISABLE;
    float bumpMat[4] = { 0, 0, 0, 0 };
    float bumpLumScale = 1.0f, bumpLumOffset = 0.0f;
    GfxTexture *texture = nullptr;
};

// Everything a draw depends on, copied into each recorded draw.
struct GfxDrawState {
    GfxMatrix world, view, projection, texture[2];
    GfxLight lights[8];
    bool lightEnabled[8] = {};
    GfxMaterial material;
    bool lighting = true;
    DWORD ambient = 0;
    bool specular = false;
    bool localViewer = true;
    bool normalizeNormals = false;
    bool colorVertex = true;
    int diffuseSource = GFX_MCS_COLOR1, specularSource = GFX_MCS_COLOR2;
    int ambientSource = GFX_MCS_MATERIAL, emissiveSource = GFX_MCS_MATERIAL;
    bool fog = false;
    DWORD fogColor = 0;
    int fogVertexMode = GFX_FOG_NONE, fogTableMode = GFX_FOG_NONE;
    float fogStart = 0.0f, fogEnd = 1.0f;
    float fadeStart = 0.0f, fadeEnd = 0.0f;   // Gfx_SetDistanceFade (end <= start: off)
    bool depthTest = true, depthWrite = true;
    int depthFunc = GFX_CMP_LESSEQUAL;
    int cull = GFX_CULL_CCW;
    bool alphaBlend = false;
    int srcBlend = GFX_BLEND_ONE, destBlend = GFX_BLEND_ZERO;
    bool alphaTest = false;
    int alphaFunc = GFX_CMP_ALWAYS;
    DWORD alphaRef = 0;
    DWORD textureFactor = 0xffffffff;
    unsigned objectId = 0; // dynamic object (car slot + 1), 0 = static scenery
    float bodyDirt[4] = {}; // amount, soil RGB; only explicitly qualified paint draws
    GfxStageState stages[2];
    GfxViewport viewport;
    int screenW = 0, screenH = 0;   // Gfx_SetVirtualScreen (0: the back buffer's size)
};

enum GfxCommandType {
    GFX_CMD_DRAW,
    GFX_CMD_CLEAR,
    GFX_CMD_TARGET,
    GFX_CMD_DUST_VOLUME,
    GFX_CMD_WORLD_END,          // explicit boundary before HUD/menu sprite layers
    GFX_CMD_BACKDROP            // copy the back buffer for the distance fade
};

struct GfxCommand {
    GfxCommandType type;
    bool raceWorld = false; // meaningful only at the world-end marker
    unsigned playerObject = 0; // world-end marker: objectId (car slot + 1) of the view's car, 0 = none (debug object mask)
    float clouds[8] = {}; // coverage, layer base, sun direction xyz, time, reserved
    // draw
    int primitive;              // list/strip types only (fans are converted)
    bool transformed;           // GfxTLVertex data
    GfxVertexBuffer *buffer;    // vertices in this buffer, else in the frame's data
    uint32_t firstVertexByte;   // into the buffer or the frame's vertex data,
                                // a multiple of the vertex size
    uint32_t vertexCount;
    uint32_t firstIndex;        // into the frame's indices, or ~0 for none
    uint32_t indexCount;
    uint32_t state;             // index into the frame's states
    unsigned dustOffset = 0;
    int dustCount = 0;
    // clear
    DWORD clearFlags;
    DWORD clearColor;
    float clearZ;
    GfxViewport clearViewport;
    int clearScreenW, clearScreenH; // the draw state's screenW, screenH
    // target
    GfxTexture *target;         // NULL: the back buffer
};

struct GfxFrame {
    std::vector<GfxDustLobe> dustLobes;
    std::vector<uint8_t> vertices;
    std::vector<uint16_t> indices;
    std::vector<GfxDrawState> states;
    std::vector<GfxCommand> commands;
    std::vector<GfxVertexBuffer *> buffers;     // drawn from, each once
    void Reset()
    {
        dustLobes.clear();
        vertices.clear();
        indices.clear();
        states.clear();
        commands.clear();
        buffers.clear();
    }
};

struct GfxReadback {
    bool requested = false;
    int x = 0, y = 0, width = 0, height = 0;
    bool ready = false;
    int readyWidth = 0, readyHeight = 0;
    std::vector<uint32_t> pixels;   // 0xAARRGGBB
};

// ---- executor -----------------------------------------------------------------------

struct GfxExecutor {
    virtual ~GfxExecutor() {}
    virtual bool SetVideoMode(int width, int height, bool fullscreen) = 0;
    virtual void SetFullscreen(bool fullscreen) = 0;
    // OpenCMR2: the anisotropic filtering level (1: off); called when the
    // setting changes at runtime, between frames.
    virtual void SetAnisotropy(int) {}
    // OpenCMR2: motion blur between frames. `enabled` turns it on/off and
    // `mix` (0..255) is how much of the previous frame to blend into the
    // current one. Called when the setting changes, between frames.
    virtual void SetMotionBlur(BOOL, int) {}
    // OpenCMR2: bloom. `enabled` turns it on/off and `intensity` (0..255)
    // scales how strongly the blurred bright pixels are added back.
    virtual void SetBloom(BOOL, int) {}
    // OpenCMR2: particles (rain/snow). `type` 0=off, 1=rain, 2=snow;
    // `intensity` (0..255) scales the emission rate.
    virtual void SetParticles(int, int) {}
    virtual bool SupportsDustVolume() {return false;}
    // OpenCMR2: directional shadows. `type` 0=off, 1=on; `bias` (0..100)
    // is the depth bias in hundredths of a depth unit.
    virtual void SetShadows(int, int) {}
    // OpenCMR2: lens/camera "grime" post-process effects, applied after the
    // scene is rendered. `video.chromatic_aberration` (0..100) is the strength
    // of color fringing toward screen edges. `video.vignette` (0..100) is how
    // much the corners and edges darken. `video.film_grain` (0..100) adds a
    // subtle noise overlay for grit. SDL_GPU only.
    virtual void SetChromaticAberration(int) {}
    virtual void SetVignette(int) {}
    virtual void SetFilmGrain(int) {}
    // OpenCMR2: depth of field. `enabled` turns it on/off, `focalDistance`
    // (0..100) is where the focus plane sits in depth, `focalRange` (0..100)
    // is how wide the in-focus band is, `maxBlur` (0..100) is the blur radius.
    virtual void SetDepthOfField(BOOL, int, int, int) {}
    virtual void SetSpeedFocus(float) {}
    virtual void SetTunnelFocus(float, float, int) {}
    virtual void ClearSpeedEffects() {}
    virtual void SetSpeedEffectsSuppressed(bool) {}
    virtual void ClearVaporTrail() {}
    virtual void SetSpeedStreak(float) {}
    virtual void SetVaporTrail(float) {}
    // OpenCMR2: god rays / volumetric lighting. `enabled` turns it on/off,
    // `intensity` (0..255) scales the effect strength.
    virtual void SetGodRays(BOOL, int) {}
    // OpenCMR2: atmosphere fog (video.fog). `amount` (0..200) scales density,
    // `glow` (0..100) the sun-ward brightening. See render/fog_math.h.
    virtual void SetPostFog(BOOL, int, int) {}
    virtual void SetPostFogClearDistance(int) {}
    // Normalized top-left screen position of actual stage sun; false hides rays.
    virtual void SetSunScreenPosition(float, float, BOOL) {}
    // OpenCMR2: car lights the fog pass responds to (video.fog_lights). Up to 8.
    virtual void SetFogLights(const GfxFogLight *, int) {}
    // Map resolution 512/1024/2048; PCF softness 0..100 maps to radius 0..4 texels.
    virtual void SetShadowQuality(int, int) {}
    // OpenCMR2: color grading. `temperature` (-100..100) cool->warm,
    // `tint` (-100..100) green->magenta, `saturation` (0..200) color strength,
    // `contrast` (0..200) contrast around middle gray.
    virtual void SetColorGrading(int, int, int, int) {}
    // Runs the recorded commands; with present, shows the back buffer and
    // ends the frame (otherwise the next commands continue it).
    virtual void Execute(GfxFrame &frame, std::vector<GfxTexture *> &textures, GfxReadback &readback, bool present,
                         bool vsync, bool stretch = false) = 0;
    virtual void DestroyTexture(GfxTexture *texture) = 0;
    virtual void DestroyVertexBuffer(GfxVertexBuffer *) {}
    // Brings a render target's pixels back into its CPU copy (for locks).
    virtual void ReadTexture(GfxTexture *texture) = 0;
    // Completes the back buffer read the last presented frame started, when
    // the game asks for its pixels (Gfx_ReadBackBuffer).
    virtual void CollectReadback(GfxReadback &) {}
};

GfxExecutor *Gfx_CreateGpuExecutor(void);
GfxExecutor *Gfx_CreateNullExecutor(void);
// Whether SDL_GPU has a hardware device on this system (gfx_gpu.cpp).
bool Gfx_GpuIsAvailable(void);
#ifdef OPENCMR2_RENDERER_GL
GfxExecutor *Gfx_CreateGlExecutor(void);
// Sets up an OpenGL 3.3 context's attributes; returns the window flags it
// needs (gfx_gl.cpp).
unsigned long long Gfx_PrepareGlWindow(void);
#endif

// Shared by the executors (gfx_core.cpp).
struct SDL_Window;
// Fullscreen on the desktop resolution, or a centred window of the back
// buffer's size times video.window_scale.
void Gfx_SetWindowMode(SDL_Window *window, bool fullscreen, int width, int height);
// OPENCMR2_FRAME_HASH=file: one line per presented frame, its number, size
// and a hash of the back buffer's pixels, to compare renderers and renderer
// changes; OPENCMR2_FRAME_DUMP=dir also saves every OPENCMR2_FRAME_DUMP_EVERY'th
// frame (default 600) as a BMP. Reading the pixels waits for the GPU: not for
// timing.
bool Gfx_FrameHashWanted(void);
void Gfx_HashFrame(const uint8_t *bgra, int width, int height);

// Decodes a BC1/BC3 4x4 block into 16 BGRA pixels.
void Gfx_DecodeBlock(int format, const uint8_t *block, uint32_t out[16]);

#endif
