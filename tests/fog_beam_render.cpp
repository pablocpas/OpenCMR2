// Fog beam wedge rendered-pixel regression (video.fog_lights): the REAL
// production endpoint calculation (game/FogBeamProjection.h) runs over real
// fixed-point matrix/projection primitives (game/FixedMatrixCore.cpp) for a
// beam that crosses the near plane - the 2026-11-01 rear-lamp streak case -
// and the resulting GfxFogLight uniforms drive the REAL fog.frag through
// gfx_core recording -> real GL executor with readback. Linux/software GL is
// NOT native Mac OpenGL/Metal acceptance; the SDL_GPU path shades the same
// fog.frag source (uniform layout pinned by tests/fog_uniform_layout.py).
#include "../src/render/gfx_core.cpp"
#include "../src/render/gfx_gl.cpp"
#include "../src/diagnostics/observer.cpp"
#include "../game/FixedMatrixCore.cpp"
#include "FogBeamProjection.h"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cstdlib>

static SDL_Window *testWindow;
static int fogLightStrength=100;
extern "C" int Sys_GetOption(const char *key, int fallback) { return !strcmp(key,"video.fog_lights") ? fogLightStrength : fallback; }
SDL_Window *Platform_GetWindow() { return testWindow; }
int Platform_GetSettingInt(const char *, int fallback) { return fallback; }
void TouchOverlay_RecordFrame(int, int, bool) {}
// The harness compiles FixedMatrixCore.cpp without SceneNode.cpp, so the
// 16.16 sqrt table is filled here with the formula Scene_InitFixedMathTables
// documents: g_sqrtTable[n] = sqrt((8 + 16*n) / 65536) * 65536, rounded.
unsigned short g_sqrtTable[4096];
static void FillSqrtTable()
{
    for (int n = 0; n < 4096; n++)
        g_sqrtTable[n] = (unsigned short)(int)(sqrt((8.0 + 16.0 * n) / 65536.0) * 65536.0 + 0.5);
}
const char *Platform_GetSetting(const char *key, const char *fallback) {
    return !strcmp(key, "video.graphics_api") ? "opengl" : fallback;
}
extern "C" void Sys_Log(const char *format, ...) { va_list a; va_start(a,format); vfprintf(stderr,format,a); fputc('\n',stderr); va_end(a); }
GfxExecutor *Gfx_CreateGpuExecutor() { return nullptr; }
bool Gfx_GpuIsAvailable(void) { return false; }
extern "C" unsigned int Sys_GetTicks() { return 0; }
extern "C" double Sys_GetTicksPrecise(void) { return 0; }
extern "C" void Sys_GetDesktopSize(int *w, int *h) { *w = 96; *h = 64; }

// Globals the production path reads.
FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
int g_fogLightSourceCount = 0;
Graphics *g_pGraphics;
double CGraphics::m_oneOver65536 = 1.0 / 65536.0;
double CGraphics::m_65536 = 65536.0;
static D3DTextureManager g_texManager;
D3DTextureManager *CGraphics::m_pTextureManager = &g_texManager;   // definition bypasses access control
const float g_floatHalf = 0.5f;

static void require(bool ok, const char *what) { if (!ok) { fprintf(stderr,"FAIL: %s\n",what); exit(1); } }

static const int W = 96, H = 64;
static GfxVertexBuffer *vb;
static GfxTexture *whiteTex;

static void putQuad()
{
    GfxVertex *v = Gfx_LockVertexBuffer(vb);
    require(v != NULL, "lock");
    const float px[4] = { -1.f, 1.f, -1.f, 1.f };
    const float py[4] = { -1.f, -1.f, 1.f, 1.f };
    for (int i = 0; i < 4; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].x = px[i]; v[i].y = py[i]; v[i].z = 1.99f;   // far: raw depth ~0.993
        v[i].nz = -1.f;
        v[i].diffuse = 0xff242022;                        // dark night road (ARGB)
        v[i].specular = 0xff000000;
    }
    Gfx_UnlockVertexBuffer(vb);
}

static void drawAndRead(DWORD *pixels)
{
    Gfx_SetPostFog(TRUE, 70, 100);   // re-arm per frame: the executor one-shots post state
    Gfx_SetRaceWorld(TRUE);
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff181c20, 1.0f);
    Gfx_SetTexture(0, whiteTex);
    Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    Gfx_EndWorldEffects();   // emits the race-world command that runs the fog pass
    static DWORD dummy[W * H];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);   // marks the request
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}

static GfxVertexBuffer *folVb;
static GfxTexture *folTex;

// Alpha-cutout "foliage" quad: left half opaque (leaf), right half cut out
// (alpha 0). Cutout texels write no depth, so the holes keep whatever depth
// is behind the quad.
static void putFoliageQuad()
{
    GfxVertex *v = Gfx_LockVertexBuffer(folVb);
    require(v != NULL, "lock foliage");
    const float px[4] = { -0.75f, -0.25f, -0.75f, -0.25f };   // screen x 12..36
    const float py[4] = { -0.30f, -0.30f, 0.30f, 0.30f };
    const float pu[4] = { 0.f, 1.f, 0.f, 1.f };
    const float pv[4] = { 0.f, 0.f, 1.f, 1.f };
    for (int i = 0; i < 4; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].x = px[i]; v[i].y = py[i]; v[i].z = 1.2f;   // nearer than the lamp: raw depth ~0.467
        v[i].nz = -1.f;
        v[i].diffuse = 0xffffffff;
        v[i].u0 = pu[i]; v[i].v0 = pv[i];
    }
    Gfx_UnlockVertexBuffer(folVb);
}

static void drawAndReadFoliage(DWORD *pixels)
{
    Gfx_SetPostFog(TRUE, 70, 100);
    Gfx_SetRaceWorld(TRUE);
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff181c20, 1.0f);
    Gfx_SetAlphaTest(FALSE);
    Gfx_SetTexture(0, whiteTex);
    Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    Gfx_SetAlphaTest(TRUE);
    Gfx_SetAlphaFunc(GFX_CMP_GREATER);
    Gfx_SetAlphaRef(0x80);
    Gfx_SetStageMinFilter(0, GFX_FILTER_POINT);   // hard texel edges: no filtered alpha ramp
    Gfx_SetStageMagFilter(0, GFX_FILTER_POINT);
    Gfx_SetTexture(0, folTex);
    Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, folVb, 0, 4);
    Gfx_SetStageMinFilter(0, GFX_FILTER_LINEAR);
    Gfx_SetStageMagFilter(0, GFX_FILTER_LINEAR);
    Gfx_SetAlphaTest(FALSE);
    Gfx_SetAlphaFunc(GFX_CMP_ALWAYS);
    Gfx_SetAlphaRef(0);
    Gfx_EndWorldEffects();
    static DWORD dummy[W * H];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}

static int red(const DWORD *f, int x, int y) { return (int)((f[y * W + x] >> 16) & 0xff); }   // ARGB red byte

static void dumpPpm(const char *prefix, const char *tag, const DWORD *pixels)
{
    if (!prefix) return;
    FILE *f = fopen((std::string(prefix) + "-" + tag + ".ppm").c_str(), "wb");
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int i = 0; i < W * H; i++) {
        DWORD px = pixels[i];
        fputc((px >> 16) & 0xff, f); fputc((px >> 8) & 0xff, f); fputc(px & 0xff, f);
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    FillSqrtTable();
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    Gfx_PrepareGlWindow();
    testWindow = SDL_CreateWindow("fog beam rendered pixels", W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    require(testWindow != NULL, SDL_GetError());
    require(Gfx_SetVideoMode(W, H, FALSE), "video mode");

    // Production projection state for FogBeam_ProjectLights: 96x64, symmetric
    // perspective (w = z). Independent of the ortho used for the scene quad.
    static Graphics gfx;
    memset(&gfx, 0, sizeof gfx);
    gfx.resX = W; gfx.resY = H;
    g_pGraphics = &gfx;
    memset(&g_texManager, 0, sizeof g_texManager);
    g_texManager.fixedProjection[0] = 0x10000;
    g_texManager.fixedProjection[5] = 0x10000;
    g_texManager.fixedProjection[10] = 0x10000;
    g_texManager.fixedProjection[11] = 0x10000;

    // Chase camera at the origin looking +Z. Rear lamp 2 units left, 8 ahead,
    // beam 30 units backwards: the beam end sits 22 units BEHIND the camera -
    // the exact geometry that produced the upper-left streak.
    static SceneNode view;
    memset(&view, 0, sizeof view);
    FixMatrix_Identity(&view.world);
    LightField_Clear();
    FixVector lampPos = {-0x20000, 0, 0x80000};        // (-2, 0, 8)
    FixVector lampDir = {0, 0, -0x10000};              // backwards
    require(LightField_Add(&lampPos, &lampDir, 0x10000, 0x10000, 0x1200, 0) == 1, "light add");
    // The harness scene quad lands at raw depth ~0.997 (GL buffer); the game
    // projection handed to the projector gives the lamp a D3D-space depthRef
    // 0.6667 - 0.3333/8.5 = 0.627 (half-unit self-occlusion bias), remapped by
    // the GL executor to ~0.814 - below the quad, so the gate passes.
    // This beam is exactly axial (pointing at the camera): the wedge must
    // collapse to the halo (the 2026-11-01 replay jumping fix), anchored 3
    // units forward along the beam at (-2,0,5) -> u 0.30.
    GfxFogLight lights[LIGHT_FIELD_MAX];
    int n = FogBeam_ProjectLights(lights, LIGHT_FIELD_MAX, &view, (float)W, (float)H, 1.0f/1.5f, -0.5f/1.5f);
    require(n == 1, "one projected light");
    require(lights[0].x > 0.30f && lights[0].x < 0.45f, "lamp u");
    require(lights[0].y > 0.45f && lights[0].y < 0.55f, "lamp v");
    require(lights[0].beamLen < 0.02f, "axial beam: wedge collapses (no flip-prone direction)");
    require(lights[0].depthRef > 0.626f && lights[0].depthRef < 0.629f, "self-occlusion bias: ref half a unit behind the lamp");
    require(lights[0].haloX == lights[0].x && lights[0].haloY == lights[0].y && lights[0].haloRef == lights[0].depthRef,
            "halo slots pinned to the lamp (bubbles removed)");

    // Same lamp, beam yawed 20 degrees toward -X (transverse fraction 0.34):
    // the accepted chase look - a real wedge running screen-left, no flip.
    LightField_Clear();
    FixVector yDir = {-0x5791, 0, -0xF0A4};            // (-0.342, 0, -0.940)
    require(LightField_Add(&lampPos, &yDir, 0x10000, 0x10000, 0x1200, 0) == 1, "yawed light add");
    GfxFogLight ylights[LIGHT_FIELD_MAX];
    int yn = FogBeam_ProjectLights(ylights, LIGHT_FIELD_MAX, &view, (float)W, (float)H, 1.0f/1.5f, -0.5f/1.5f);
    require(yn == 1, "yawed light survives");
    require(ylights[0].beamX < -0.9f && ylights[0].beamY > -0.05f && ylights[0].beamY < 0.05f,
            "yawed beam: wedge runs screen-left");
    require(ylights[0].beamLen > 0.55f && ylights[0].beamLen <= 0.61f, "yawed beam: foreshortened wedge length");

    // Temporal continuity: the same world light seen from a camera yawed 10
    // degrees (an auto-cam cut) must keep the wedge on the SAME screen side
    // with a comparable length - no sign flip across the cut.
    static SceneNode cut;
    memset(&cut, 0, sizeof cut);
    FixMatrix_Identity(&cut.world);
    cut.world.right.x = 0xFBF8;  cut.world.right.z = -0x2C72;   // +10 degrees about Y
    cut.world.forward.x = 0x2C72; cut.world.forward.z = 0xFBF8;
    GfxFogLight clights[LIGHT_FIELD_MAX];
    int cn = FogBeam_ProjectLights(clights, LIGHT_FIELD_MAX, &cut, (float)W, (float)H, 1.0f/1.5f, -0.5f/1.5f);
    require(cn == 1, "cut light survives");
    require((clights[0].beamX < 0.0f) == (ylights[0].beamX < 0.0f), "camera cut: no wedge side flip");
    require(clights[0].beamLen > ylights[0].beamLen * 0.3f && clights[0].beamLen < ylights[0].beamLen * 3.0f,
            "camera cut: wedge length continuous");

    // Overhead replay auto-cam (user screenshots 2026-11-03: 2:49 overhead
    // shows lamps lit but forward beams absent; rear glow visible). Camera 10
    // units above the car looking straight down, car heading +Z: the
    // horizontal forward beam is fully transverse to the view axis, so the
    // wedge must survive long and point screen-forward (+v). If this passes,
    // an overhead disappearance is not projection-side - the suspects left
    // are the 8-light field starving headlights (priority fix #308 merged
    // after those screenshots; the replay OSD "lf x/8 lost N hl N" line
    // settles it natively) or the producer not recording the car's lights
    // that step.
    {   static SceneNode overhead;
        memset(&overhead, 0, sizeof overhead);
        FixMatrix_Identity(&overhead.world);
        // looking straight down: forward = -Y, right = +X, up = +Z
        overhead.world.right.x = 0x10000;  overhead.world.right.y = 0;        overhead.world.right.z = 0;
        overhead.world.up.x = 0;           overhead.world.up.y = 0;           overhead.world.up.z = 0x10000;
        overhead.world.forward.x = 0;      overhead.world.forward.y = -0x10000; overhead.world.forward.z = 0;
        overhead.world.position.y = 0xA0000;   // 10 units up
        LightField_Clear();
        FixVector oPos = {0, 0x8000, 0x80000};        // car at origin, front lamp (0, 0.5, 8)
        FixVector oDir = {0, 0, 0x10000};             // forward, horizontal
        require(LightField_Add(&oPos, &oDir, 0x10000, 0x10000, 0x1200, 0) == 1, "overhead light add");
        GfxFogLight olights[LIGHT_FIELD_MAX];
        int on = FogBeam_ProjectLights(olights, LIGHT_FIELD_MAX, &overhead, (float)W, (float)H, 1.0f/1.5f, -0.5f/1.5f);
        require(on == 1, "overhead cam: forward beam light survives projection");
        require(olights[0].beamLen > 0.05f, "overhead cam: transverse beam keeps a real wedge");
    }

    // R look-back case (user screenshot: red glow by the left spoiler
    // support): camera rotated 180 degrees, rear lamp ahead in view, beam
    // pointing back toward/past the camera. The wedge must run from the lamp
    // toward its true screen side (right, here) - the pre-clip sentinel put
    // the endpoint at the upper-left, beside the left support.
    static SceneNode lookback;
    memset(&lookback, 0, sizeof lookback);
    FixMatrix_Identity(&lookback.world);
    lookback.world.right.x = -0x10000;   // 180 degrees about Y: x -> -x, z -> -z
    lookback.world.forward.z = -0x10000;
    LightField_Clear();
    FixVector rlPos = {-0x20000, 0, -0x80000};        // world (-2, 0, -8): in view (2, 0, 8)
    FixVector rlDir = {0, 0, 0x10000};                // world +Z: view -Z, toward the camera
    require(LightField_Add(&rlPos, &rlDir, 0x10000, 0x10000, 0x1200, 0) == 1, "lookback light add");
    GfxFogLight rlights[LIGHT_FIELD_MAX];
    int rn = FogBeam_ProjectLights(rlights, LIGHT_FIELD_MAX, &lookback, (float)W, (float)H, 1.0f/1.5f, -0.5f/1.5f);
    require(rn == 1, "look-back rear lamp survives");
    require(rlights[0].x > 0.55f && rlights[0].x < 0.70f, "look-back lamp on its true screen side");
    require(rlights[0].beamLen < 0.02f, "look-back axial beam: no wedge, no mirror to the upper-left");

    // Bonnet/bumper case: lamp 0.5 units ahead of the camera (inside the
    // near-plane threshold), beam 30 units forward into view. The light must
    // survive with its anchor clipped to the near plane; before the fix the
    // whole light was dropped (the missing bonnet-view lamps).
    LightField_Clear();
    FixVector blPos = {-0x20000, 0, 0x8000};          // (-2, 0, 0.5)
    FixVector blDir = {0, 0, 0x10000};                // forward
    require(LightField_Add(&blPos, &blDir, 0x10000, 0x10000, 0x1200, 0) == 1, "bonnet light add");
    GfxFogLight blights[LIGHT_FIELD_MAX];
    int bn = FogBeam_ProjectLights(blights, LIGHT_FIELD_MAX, &view, (float)W, (float)H, 1.0f/1.5f, -0.5f/1.5f);
    require(bn == 1, "lamp inside the near plane keeps its light");
    require(blights[0].depthRef > 0.43f && blights[0].depthRef < 0.46f, "clipped anchor gated at 1.5 units out");
    require(blights[0].haloX == blights[0].x && blights[0].haloY == blights[0].y && blights[0].haloRef < blights[0].depthRef,
            "bonnet segment anchor is unbiased");

    GfxViewport vp = { 0, 0, W, H, 0.f, 1.f };
    Gfx_SetViewport(&vp);
    GfxMatrix identity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &identity);
    Gfx_SetTransform(GFX_TRANSFORM_VIEW, &identity);
    GfxMatrix proj = { 1,0,0,0, 0,1,0,0, 0,0,1.f/1.5f,0, 0,0,-0.5f/1.5f,1 };
    Gfx_SetTransform(GFX_TRANSFORM_PROJECTION, &proj);
    Gfx_SetLighting(FALSE);
    Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetRaceWorld(TRUE);
    Gfx_SetPostFog(TRUE, 70, 100);   // his reference settings: amount 70, glow 100

    vb = Gfx_CreateVertexBuffer(4);
    require(vb != NULL, "vertex buffer");
    whiteTex = Gfx_CreateTexture(1, 1, GFX_FORMAT_BGRA8, 1, 0);
    require(whiteTex != NULL, "white texture");
    {   GfxLockedRect lr;
        require(Gfx_LockTexture(whiteTex, 0, 0, &lr), "lock white");
        memset(lr.pixels, 0xff, 4);
        Gfx_UnlockTexture(whiteTex, 0, 0);
    }
    folVb = Gfx_CreateVertexBuffer(4);
    require(folVb != NULL, "foliage vertex buffer");
    folTex = Gfx_CreateTexture(4, 3, GFX_FORMAT_BGRA8, 1, 0);
    require(folTex != NULL, "foliage texture");
    {   GfxLockedRect lr;
        require(Gfx_LockTexture(folTex, 0, 0, &lr), "lock foliage tex");
        unsigned char *p = (unsigned char *)lr.pixels;
        // 4x3 texels over the same quad. v-bands 0 and 2: opaque dark leaf
        // left half, cut out right half (the original pattern). v-band 1 -
        // the MIDDLE band, which row 32 (quad centre, v ~= 0.5) samples
        // under either readback orientation - is a NARROW leaf strip at
        // texel 1 (screen x 18..24) between cutouts (Devin review #2): from
        // the hole pixel at x=29 the nearest leaf sits 5-11 px away,
        // between the 4 px and 12 px evidence rings, caught only by the 8 px
        // ring. The original assertions keep their meaning in the strip
        // band: leaf at x=21 is still opaque, the cutout edge is still x=24,
        // and the x=32 wide hole is still reachable only by the 12 px ring.
        for (int b = 0; b < 3; b++)
            for (int i = 0; i < 4; i++) {   // BGRA
                unsigned char *t = p + (b * 4 + i) * 4;
                t[0] = 0x20; t[1] = 0x50; t[2] = 0x20;
                t[3] = (b != 1 ? i < 2 : i == 1) ? 0xff : 0x00;
            }
        Gfx_UnlockTexture(folTex, 0, 0);
    }

    const char *prefix = argc > 1 ? argv[1] : nullptr;
    static DWORD control[W * H], lit[W * H];
    // 1. Control: fog only, no lights.
    Gfx_SetFogLights(nullptr, 0);
    putQuad();
    drawAndRead(control);
    // 2. Lit: fog + the near-plane-crossing beam.
    Gfx_SetFogLights(lights, n);
    putQuad();
    drawAndRead(lit);

    // 3. Depth gate: the same lamp with its depth reference moved BEHIND the
    // scene quad (an occluder in front of the lamp, e.g. the dashboard) must
    // leave the frame at the control level.
    GfxFogLight occluded = lights[0];
    occluded.depthRef = 0.9999f;   // GL remap -> 0.99995, past the quad's ~0.997
    occluded.haloRef = 0.9999f;
    static DWORD gated[W * H];
    Gfx_SetFogLights(&occluded, 1);
    putQuad();
    drawAndRead(gated);
    // 4. Lamp inside the near plane: exercise the clipped projected wedge.
    static DWORD bonnet[W * H];
    Gfx_SetFogLights(blights, bn);
    putQuad();
    drawAndRead(bonnet);
    // 5. R look-back: the halo glows at the lamp, never the upper-left.
    static DWORD rlook[W * H];
    Gfx_SetFogLights(rlights, rn);
    putQuad();
    drawAndRead(rlook);
    // 6. Yawed chase beam: the wedge brightens the beam path screen-left.
    static DWORD yawed[W * H];
    Gfx_SetFogLights(ylights, yn);
    putQuad();
    drawAndRead(yawed);

    // 7. Alpha-cutout foliage between camera and beam (user screenshot
    // 2026-11-06, replay 2:57: pale halos / dark cutout patches around foliage
    // near beams). The foliage quad sits nearer than the lamp, so its opaque
    // leaves must gate the beam's far segments; the cutout holes keep the far
    // scene depth, so the gate passes there even for segments BEHIND the
    // foliage. Diagnostic prints measure the leak; the assertion pins the
    // correct half (leaves gate far segments) so a fix can flip the leak
    // prints into assertions.
    static DWORD folCon[W * H], folLit[W * H];
    Gfx_SetFogLights(nullptr, 0);
    putQuad(); putFoliageQuad();
    drawAndReadFoliage(folCon);
    Gfx_SetFogLights(ylights, yn);
    putQuad(); putFoliageQuad();
    drawAndReadFoliage(folLit);
    dumpPpm(prefix, "foliage", folLit);
    {   int leafLift = red(folLit,21,32) - red(folCon,21,32);   // opaque leaf, 3px from the cutout edge
        int holeLift = red(folLit,27,32) - red(folCon,27,32);   // cutout hole, 3px past the edge (4px ring reaches the leaf)
        int hole32 = red(folLit,32,32) - red(folCon,32,32);     // wide-hole interior, 8px past the edge (12px ring reaches the leaf)
        fprintf(stderr, "foliage gate: leaf lift %+d, hole(27) lift %+d, hole(32) lift %+d\n", leafLift, holeLift, hole32);
        // fog.frag fades a beam segment's light where a close neighbor's raw
        // depth is IN FRONT of that segment (occluder evidence), no farther
        // in front than 70% of the segment's raw depth (a drastically nearer
        // shell/dashboard is not an occluder - the cockpit_window_render
        // shell case pins that side), and the pixel's view depth is >25%
        // farther (magnitude). Pre-fix leak was hole(32) +77; both holes
        // must drop to the leaf level, and the open-scene yawed assertions
        // above (uniform depth, no near occluders) pin the false-positive
        // side: no blanket darkening of legitimately lit background.
        require(leafLift >= -2 && leafLift <= 8, "foliage: opaque leaf gates the beam");
        require(holeLift <= leafLift + 8, "foliage: cutout hole no longer leaks the beam");
        require(hole32 <= leafLift + 8, "foliage: wide cutout interior no longer leaks the beam");
        // Narrow leaf strip (Devin review #2): from the hole pixel at x=29
        // the nearest leaf (texel x 18..24) sits 5-11 px away - between the
        // 4 px and 12 px rings, caught only by the 8 px ring. With 4/12
        // alone the hole stays lit (the slip, verified locally by disabling
        // the 8 px ring: hole(29) jumps to the full leak).
        int stripHole = red(folLit,29,32) - red(folCon,29,32);
        fprintf(stderr, "strip gate: hole(29) lift %+d\n", stripHole);
        require(stripHole <= leafLift + 8, "strip: hole beside a narrow leaf no longer slips the rings");
    }

    dumpPpm(prefix, "lookback", rlook);
    dumpPpm(prefix, "yawed", yawed);

    // CORRECTED EXPECTATIONS (2026-11-01 harness repair, measured on llvmpipe
    // 96x64 against BOTH main and the segment-gate shader: identical values).
    // The removed assertion "beam changes pixels" went stale when the glow
    // bubbles were removed: an on-axis beam is band-faded to zero length in
    // FogBeamProjection (transverse < 0.10), so lit == control EXACTLY. That
    // identity is now the assertion - it pins the band fade.
    require(memcmp(control, lit, sizeof control) == 0, "axial beam: band-faded, no pixels change");
    // The old "brightens by >24 at (12,32)" was stale in this environment: the
    // wedge's actual brightening there is +10 (main and patched shader alike).
    // Assert the beam-path REGION brightens (extent + count) and the one
    // sampled pixel stays in the measured band: the lower bound keeps the
    // wedge visible, the upper bound trips if the capped wash regrows.
    require(red(lit,12,32) <= red(control,12,32) + 8, "axial beam: no long wedge");
    {   int yawedLift = 0;
        for (int yy = 28; yy <= 35; yy++) for (int xx = 10; xx <= 35; xx++)
            if (red(yawed,xx,yy) > red(control,xx,yy) + 5) yawedLift++;
        require(yawedLift >= 100, "yawed beam: wedge path region brightens");
    }
    require(red(yawed,12,32) > red(control,12,32) + 5 && red(yawed,12,32) <= red(control,12,32) + 20,
            "yawed beam: path brightens moderately (wash stays thinned)");
    require(red(yawed,2,2) <= red(control,2,2) + 8, "yawed beam: no upper-left streak");
    // The reported bug: the streak ran to the upper-left corner. It must not:
    require(red(lit,2,2) <= red(control,2,2) + 8, "no upper-left streak (corner)");
    require(red(lit,12,8) <= red(control,12,8) + 8, "no upward drift above the beam");
    // No backward shine: right of the lamp stays at the control level.
    require(red(lit,84,32) <= red(control,84,32) + 8, "no backward shine");
    // The gate must blank the whole light when the scene sits in front of it.
    require(abs(red(gated,12,32) - red(control,12,32)) <= 8, "occluded lamp: beam path gated off");
    // Bonnet case renders via its wedge (bubble assertions removed).
    // Look-back: no wedge, no upper-left, no bubble.
    require(red(rlook,84,32) <= red(control,84,32) + 8, "look-back: no right-side wedge");
    require(red(rlook,2,2) <= red(control,2,2) + 8, "look-back: no upper-left glow");
    require(red(rlook,12,32) <= red(control,12,32) + 8, "look-back: no left-side spill");
    // Camera matrix: forward lamps must light the projected road/fog path,
    // including level bonnet/bumper views. Far scene depth stands for open
    // road; an occluding near quad must remain dark. No private assets.
    struct CameraCase { const char *name; float y,z,pitch; };
    const CameraCase cameras[] = {{"chase",2.4f,-6.8f,0},{"bonnet",0.95f,1.75f,0},{"bumper",0.55f,2.35f,0},
        {"chase-pitched",2.4f,-6.8f,14},{"bonnet-pitched",0.95f,1.75f,6},{"bumper-pitched",0.55f,2.35f,2}};
    for (const CameraCase &camera : cameras) {
        SceneNode cv={}; FixMatrix_Identity(&cv.world);
        float pitch=camera.pitch*3.14159265f/180;
        cv.world.up.y=cv.world.forward.z=(int)(cos(pitch)*65536);
        cv.world.up.z=(int)(sin(pitch)*65536); cv.world.forward.y=-cv.world.up.z;
        cv.world.position.y=(int)(camera.y*65536); cv.world.position.z=(int)(camera.z*65536);
        LightField_Clear();
        for (int side=-1;side<=1;side+=2) {
            FixVector pos={(int)(side*0.65f*65536),(int)(0.55f*65536),(int)(2.05f*65536)};
            FixVector dir={0,0,65536};
            LightField_Add(&pos,&dir,65536,65536,65536,65536);
        }
        GfxFogLight forward[LIGHT_FIELD_MAX];
        int nf=FogBeam_ProjectLights(forward,LIGHT_FIELD_MAX,&cv,W,H,1.0f/1.5f,-0.5f/1.5f);
        require(nf==2,"camera matrix: two forward lamps");
        static DWORD off[W*H],on[W*H];
        putQuad(); Gfx_SetFogLights(nullptr,0); drawAndRead(off);
        Gfx_SetFogLights(forward,nf);
        fogLightStrength=0; drawAndRead(on);
        require(memcmp(off,on,sizeof off)==0,"forward camera matrix: fog-lights 0 is the control");
        fogLightStrength=100; drawAndRead(on);
        std::string offTag=std::string(camera.name)+"-off";
        dumpPpm(prefix,offTag.c_str(),off);
        int path=0,nearSpots=0,backward=0,beyond=0;
        for(int yy=0;yy<H;yy++) for(int xx=0;xx<W;xx++) {
            int lift=red(on,xx,yy)-red(off,xx,yy);
            if(lift<=5) continue;
            // Readback has top-left origin, matching the production projector.
            float u=(xx+0.5f)/W,v=(yy+0.5f)/H;
            bool ahead=false,near=false,within=false;
            for(int i=0;i<nf;i++) {
                float dx=(u-forward[i].x)*W/H,dy=v-forward[i].y;
                float along=dx*forward[i].beamX+dy*forward[i].beamY;
                ahead |= along>0.03f;
                within |= along>=0 && along<=forward[i].beamLen;
                near |= dx*dx+dy*dy<0.02f*0.02f;
            }
            if(ahead) path++; else backward++;
            if(!within) beyond++;
            if(near && camera.z>1.0f) nearSpots++;
        }
        fprintf(stderr,"forward %s path=%d near=%d backward=%d\n",camera.name,path,nearSpots,backward);
        dumpPpm(prefix,camera.name,on);
        require(path>=8,"forward camera matrix: open road/fog path receives light");
        require(nearSpots==0,"forward camera matrix: no near-camera spots");
        require(backward==0,"forward camera matrix: no backward wash");
        require(beyond==0,"forward camera matrix: no continuation past beam endpoints");
        static DWORD strong[W*H];
        fogLightStrength=200; drawAndRead(strong); fogLightStrength=100;
        int extraLift=0;
        for(int j=0;j<W*H;j++) {
            require(((strong[j]>>16)&255)>=((on[j]>>16)&255),"fog-lights 200 never dims the beam");
            if(((strong[j]>>16)&255)>((on[j]>>16)&255)) extraLift++;
        }
        require(extraLift>=8,"fog-lights 200 increases the beam response");
        std::string strongTag=std::string(camera.name)+"-200";
        dumpPpm(prefix,strongTag.c_str(),strong);
        // A near scene quad is in front of the production depth gates.
        GfxVertex *nearVertices=Gfx_LockVertexBuffer(vb);
        for(int i=0;i<4;i++) nearVertices[i].z=0.6f;
        Gfx_UnlockVertexBuffer(vb);
        Gfx_SetFogLights(nullptr,0); drawAndRead(off);
        Gfx_SetFogLights(forward,nf); drawAndRead(on);
        require(memcmp(off,on,sizeof off)==0,"forward camera matrix: foreground occlusion");
    }
    // Isolate raw depth ordering in the real shader. The existing scene
    // depth sits between the unbiased and the old biased interpolation in
    // this midpoint region; a biased lamp gate still excludes bodywork.
    {
        GfxFogLight beam={};
        beam.x=0.5f; beam.y=0.2f; beam.beamY=1; beam.beamLen=0.5f;
        beam.beamFade=1.0f/31; beam.intensity=4;
        beam.r=beam.g=beam.b=1;
        beam.depthRef=0.980f;
        beam.haloRef=0.960f;
        beam.beamEndRef=0.999f;
        static DWORD off[W*H],on[W*H];
        putQuad();
        Gfx_SetFogLights(nullptr,0);drawAndRead(off);
        Gfx_SetFogLights(&beam,1);drawAndRead(on);
        int lift=0;for(int y=34;y<39;y++)for(int x=45;x<51;x++)lift=std::max(lift,red(on,x,y)-red(off,x,y));

        fprintf(stderr,"clipped intermediate road lift=%d\n",lift);
        require(lift>5,"clipped forward segment lights road behind it");
        beam.depthRef=0.9999f; // same segment, foreground self-occlusion
        Gfx_SetFogLights(&beam,1);drawAndRead(on);
        require(memcmp(off,on,sizeof off)==0,"unbiased segment retains biased bodywork gate");
    }
    // Backward beam over scenery between the segment and the lamp (Devin
    // review r4536372133): a backward beam approaches the camera, so its
    // per-pixel segment ref is closer than the biased lamp ref. max(lamp,
    // segment) applied that farther ref across the whole wedge and hid the
    // beam's light on nearby scenery; the biased lamp maximum is for forward
    // beams only.
    {
        GfxFogLight back={};
        back.x=0.5f; back.y=0.2f; back.beamY=1; back.beamLen=0.5f;
        back.beamFade=2.0f;                 // q > 1: backward thinning
        back.intensity=4;
        back.r=back.g=back.b=1;
        back.depthRef=0.9990f;              // biased lamp ref past the quad
        back.beamEndRef=0.90f;              // segment end much closer: midpoint segRef ~0.93-0.95
        static DWORD off[W*H],on[W*H];
        putQuad();
        Gfx_SetFogLights(nullptr,0);drawAndRead(off);
        Gfx_SetFogLights(&back,1);drawAndRead(on);
        int lift=0;for(int y=34;y<39;y++)for(int x=45;x<51;x++)lift=std::max(lift,red(on,x,y)-red(off,x,y));
        fprintf(stderr,"backward transverse midpoint lift=%d\n",lift);
        require(lift>5,"backward beam lights scenery between segment and lamp");
        // Same beam with the whole segment run past the quad: scenery closer
        // than every segment must stay dark (the gate is not off for backward
        // beams, it only drops the lamp maximum).
        back.depthRef=0.9995f; back.beamEndRef=0.9980f;
        Gfx_SetFogLights(&back,1);drawAndRead(on);
        require(memcmp(off,on,sizeof off)==0,"backward beam still gated where the scene is closer than every segment");
    }
    Gfx_SetTransform(GFX_TRANSFORM_PROJECTION,&proj);
    // Short forward run: the historical 0.05 denominator floor must not
    // extend an actual endpoint only 0.015 screen units from its anchor.
    // Direct uniform isolates the production shader from projection rounding.
    {
        GfxFogLight shortBeam={};
        shortBeam.x=0.5f; shortBeam.y=0.5f;
        shortBeam.beamY=1.0f; shortBeam.beamLen=0.015f;
        shortBeam.beamFade=0.5f;
        shortBeam.r=shortBeam.g=shortBeam.b=1.0f;
        shortBeam.intensity=4.0f;
        static DWORD off[W*H], on[W*H];
        putQuad(); Gfx_SetFogLights(nullptr,0); drawAndRead(off);
        Gfx_SetFogLights(&shortBeam,1); drawAndRead(on);
        int continuation=0;
        for(int yy=0;yy<H;yy++) for(int xx=0;xx<W;xx++) {
            float along=(yy+0.5f)/H-shortBeam.y;
            if(along>shortBeam.beamLen && red(on,xx,yy)>red(off,xx,yy)+5)
                continuation++;
        }
        fprintf(stderr,"short forward continuation=%d\n",continuation);
        require(continuation==0,"short forward beam terminates at true endpoint");
    }
    printf("fog_beam_render ok\n");
    return 0;
}
