// Cockpit exterior-window depth-write rendered-pixel regression
// (video.cockpit_glass_fog): the REAL production decision
// (game/CockpitWindow.h: CockpitWindow_SkipDepthWrite) drives depth writes
// for a synthetic exterior-glass quad drawn through gfx_core -> the real GL
// executor, with the REAL depth-sampling fog pass (Gfx_SetPostFog +
// Gfx_EndWorldEffects) and a depth-tested synthetic crack pane behind the
// glass. Pixel readback compares: cockpit with the fix (fog applies through
// the window, crack visible), cockpit pre-fix (fog blocked, crack occluded),
// chase with the fix (pixel-identical to pre-fix), and a no-fog control.
// Linux/software GL is NOT native Mac OpenGL/Metal acceptance; the SDL_GPU
// path runs the same decision and fog shader inputs.
#include "../src/render/gfx_core.cpp"
#include "../src/render/gfx_gl.cpp"
#include "../src/diagnostics/observer.cpp"
#include "../game/Graphics.h"
#include "../game/CockpitWindow.h"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <string>

static SDL_Window *testWindow;
extern "C" int Sys_GetOption(const char *, int fallback) { return fallback; }
int Sys_GetOption(const char *, int fallback);   // port/sys.h shape (unused: the extern "C" stub above wins for C callers)
SDL_Window *Platform_GetWindow() { return testWindow; }
int Platform_GetSettingInt(const char *, int fallback) { return fallback; }
const char *Platform_GetSetting(const char *key, const char *fallback) {
    return !strcmp(key, "video.graphics_api") ? "opengl" : fallback;
}
extern "C" void Sys_Log(const char *format, ...) { va_list a; va_start(a,format); vfprintf(stderr,format,a); fputc('\n',stderr); va_end(a); }
GfxExecutor *Gfx_CreateGpuExecutor() { return nullptr; }
bool Gfx_GpuIsAvailable(void) { return false; }
extern "C" unsigned int Sys_GetTicks() { return 0; }
extern "C" double Sys_GetTicksPrecise(void) { return 0; }
extern "C" void Sys_GetDesktopSize(int *w, int *h) { *w = 96; *h = 64; }
void TouchOverlay_RecordFrame(int, int, bool) {}   // harness: no touch overlay

Graphics *g_pGraphics;
double CGraphics::m_oneOver65536 = 1.0 / 65536.0;
double CGraphics::m_65536 = 65536.0;
static D3DTextureManager g_texManager;
D3DTextureManager *CGraphics::m_pTextureManager = &g_texManager;
const float g_floatHalf = 0.5f;

static void require(bool ok, const char *what) { if (!ok) { fprintf(stderr,"FAIL: %s\n",what); exit(1); } }

static const int W = 96, H = 64;
static GfxVertexBuffer *vbWorld, *vbGlass, *vbCrack;
static GfxTexture *whiteTex;

// One full-height quad: xl..xr in TL screen space, depth z (the ortho
// projection maps z 0.5..2.0 -> raw depth 0..1), flat diffuse ARGB. One VB
// per quad, filled once: re-locking a single VB between draws of the same
// frame does not record reliably.
static void fillQuad(GfxVertexBuffer *vb, float xl, float xr, float z, DWORD diffuse)
{
    GfxVertex *v = Gfx_LockVertexBuffer(vb);
    require(v != NULL, "lock");
    const float px[4] = { xl, xr, xl, xr };
    const float py[4] = { -1.f, -1.f, 1.f, 1.f };
    for (int i = 0; i < 4; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].x = px[i]; v[i].y = py[i]; v[i].z = z;
        v[i].nz = -1.f;
        v[i].diffuse = diffuse;
        v[i].specular = 0xff000000;
    }
    Gfx_UnlockVertexBuffer(vb);
}

// The synthetic cockpit scene: dark world far away (z 1.99 -> raw depth
// ~0.995), an exterior-glass band over the left half of the screen (near,
// z 0.02 -> raw depth ~0.01) drawn with the production decision's depth-write
// outcome, and a bright "crack" strip behind the glass (z 1.5 -> raw depth
// 0.75) drawn with the production crack pane's states (depth tested, no
// depth write, alpha blend). The fog pass then samples the depth buffer.
static void drawScene(DWORD *pixels, int viewType, int optionOn, int fogOn, int shellMode = 0, bool beamOn = false)
{
    if (fogOn) Gfx_SetPostFog(TRUE, 100, 65);   // his F10 settings: amount 100, glow 65 (re-arm per frame: one-shot)
    else Gfx_SetPostFog(FALSE, 0, 0);
    GfxFogLight lamp={};lamp.x=.05f;lamp.y=.5f;lamp.intensity=1;lamp.r=1;lamp.g=1;lamp.b=.8f;
    lamp.beamX=1;lamp.beamY=0;lamp.beamLen=.6f;lamp.beamFade=1;lamp.depthRef=.8f;lamp.beamEndRef=.9f;
    Gfx_SetFogLights(beamOn?&lamp:nullptr,beamOn?1:0);
    Gfx_SetRaceWorld(TRUE);
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff101418, 1.0f);

    Gfx_SetTexture(0, whiteTex);
    // World: far, opaque, depth written. Dark forest green.
    Gfx_SetAlphaBlend(FALSE);
    Gfx_SetDepthTest(TRUE);
    Gfx_SetDepthWrite(TRUE);
    Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vbWorld, 0, 4);

    // Exterior window glass over the left half: near, translucent (alpha 64).
    // The REAL decision says whether this draw writes depth (node flag 0x14,
    // car-owned). Env overrides exist only for executor-behaviour probes.
    // Translucent states like the production glass/crack path: blend +
    // modulated texture*diffuse for BOTH color and alpha (without the alpha
    // stage the fragment is opaque and hides the interaction under test).
    bool skipDepth = CockpitWindow_SkipDepthWrite(0x14, true, viewType, optionOn, shellMode == 2);
    Gfx_SetDepthWrite(skipDepth ? FALSE : TRUE);
    Gfx_SetAlphaBlend(TRUE);
    Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);
    Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetStageColorOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageColorArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageColorArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageAlphaOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageAlphaArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageAlphaArg2(0, GFX_TA_DIFFUSE);
    if (shellMode) Gfx_SetAlphaBlend(FALSE); // opaque shell, same reused node type
    Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vbGlass, 0, 4);
    Gfx_SetDepthWrite(TRUE);

    // Crack pane strip behind the glass: production states (WindshieldGlass
    // overlay: depth tested, no depth write, src-alpha blend). Bright white.
    Gfx_SetDepthTest(TRUE);
    Gfx_SetDepthWrite(FALSE);
    if (!shellMode) Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vbCrack, 0, 4);
    Gfx_SetAlphaBlend(FALSE);

    Gfx_EndWorldEffects();   // emits the race-world command that runs the fog pass
    static DWORD dummy[W * H];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);   // marks the request
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}

static int chan(const DWORD *f, int x, int y, int shift) { return (int)((f[y * W + x] >> shift) & 0xff); }
static int lum(const DWORD *f, int x, int y) { return chan(f,x,y,16) + chan(f,x,y,8) + chan(f,x,y,0); }

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
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    Gfx_PrepareGlWindow();
    testWindow = SDL_CreateWindow("cockpit window depth rendered pixels", W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    require(testWindow != NULL, SDL_GetError());
    require(Gfx_SetVideoMode(W, H, FALSE), "video mode");

    static Graphics gfx;
    memset(&gfx, 0, sizeof gfx);
    gfx.resX = W; gfx.resY = H;
    g_pGraphics = &gfx;
    memset(&g_texManager, 0, sizeof g_texManager);

    GfxViewport vp = { 0, 0, W, H, 0.f, 1.f };
    Gfx_SetViewport(&vp);
    GfxMatrix identity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &identity);
    Gfx_SetTransform(GFX_TRANSFORM_VIEW, &identity);
    GfxMatrix proj = { 1,0,0,0, 0,1,0,0, 0,0,1.f/1.5f,0, 0,0,-0.5f/1.5f,1 };
    Gfx_SetTransform(GFX_TRANSFORM_PROJECTION, &proj);
    Gfx_SetLighting(FALSE);
    Gfx_SetCullMode(GFX_CULL_NONE);

    vbWorld = Gfx_CreateVertexBuffer(4);
    vbGlass = Gfx_CreateVertexBuffer(4);
    vbCrack = Gfx_CreateVertexBuffer(4);
    require(vbWorld != NULL && vbGlass != NULL && vbCrack != NULL, "vertex buffers");
    fillQuad(vbWorld, -1.f, 1.f, 1.99f, 0xff1a241c);       // far world
    fillQuad(vbGlass, -1.f, 0.f, 0.60f, 0x40c83c3c);       // near translucent red glass (z 0.6 -> raw depth ~0.067); red so the fog mix is unmistakable
    fillQuad(vbCrack, -0.75f, -0.25f, 1.5f, 0xc8ffffff);   // crack strip behind the glass (z 1.5 -> raw depth ~0.667)
    whiteTex = Gfx_CreateTexture(1, 1, GFX_FORMAT_BGRA8, 1, 0);
    require(whiteTex != NULL, "white texture");
    {   GfxLockedRect lr;
        require(Gfx_LockTexture(whiteTex, 0, 0, &lr), "lock white");
        memset(lr.pixels, 0xff, 4);
        Gfx_UnlockTexture(whiteTex, 0, 0);
    }

    // Sanity: the decision itself (unit-level, but here in the render path).
    require(CockpitWindow_SkipDepthWrite(0x14, true, 3, 1), "cockpit+car+0x14+option -> skip");
    require(!CockpitWindow_SkipDepthWrite(0x14, true, 3, 0), "option off -> write");
    require(!CockpitWindow_SkipDepthWrite(0x14, true, 1, 1), "chase view -> write");
    require(!CockpitWindow_SkipDepthWrite(0x00, true, 3, 1), "other node -> write");
    require(!CockpitWindow_SkipDepthWrite(0x14, false, 3, 1), "non-car node -> write");

    const char *prefix = argc > 1 ? argv[1] : nullptr;
    static DWORD cockpitFix[W * H], cockpitPre[W * H], chaseFix[W * H], noFog[W * H];
    drawScene(cockpitFix, 3, 1, 1);   // cockpit view, fix on: glass writes no depth
    drawScene(cockpitPre, 3, 0, 1);   // cockpit view, option off = pre-fix behaviour
    drawScene(chaseFix,  1, 1, 1);    // chase view, fix on: must equal pre-fix
    drawScene(noFog,     3, 0, 0);    // pre-fix scene with the fog pass off (control)

    dumpPpm(prefix, "cockpit-fixed", cockpitFix);
    dumpPpm(prefix, "cockpit-prefix", cockpitPre);
    dumpPpm(prefix, "chase-fixed", chaseFix);
    dumpPpm(prefix, "nofog-control", noFog);

    // Sample points (all quads span full height, so the row is arbitrary):
    // window glass area outside the crack strip, crack strip centre, and the
    // right half = unglassed control area.
    const int wx = 42, cx = 24, kx = 72, row = 32;

    printf("window  fixed=%3d,%3d,%3d  prefix=%3d,%3d,%3d  chase=%3d,%3d,%3d  nofog=%3d,%3d,%3d\n",
        chan(cockpitFix,wx,row,16),chan(cockpitFix,wx,row,8),chan(cockpitFix,wx,row,0),
        chan(cockpitPre,wx,row,16),chan(cockpitPre,wx,row,8),chan(cockpitPre,wx,row,0),
        chan(chaseFix,wx,row,16),chan(chaseFix,wx,row,8),chan(chaseFix,wx,row,0),
        chan(noFog,wx,row,16),chan(noFog,wx,row,8),chan(noFog,wx,row,0));
    printf("crack   fixed=%3d,%3d,%3d  prefix=%3d,%3d,%3d  nofog=%3d,%3d,%3d\n",
        chan(cockpitFix,cx,row,16),chan(cockpitFix,cx,row,8),chan(cockpitFix,cx,row,0),
        chan(cockpitPre,cx,row,16),chan(cockpitPre,cx,row,8),chan(cockpitPre,cx,row,0),
        chan(noFog,cx,row,16),chan(noFog,cx,row,8),chan(noFog,cx,row,0));
    printf("control fixed=%3d,%3d,%3d  prefix=%3d,%3d,%3d  nofog=%3d,%3d,%3d\n",
        chan(cockpitFix,kx,row,16),chan(cockpitFix,kx,row,8),chan(cockpitFix,kx,row,0),
        chan(cockpitPre,kx,row,16),chan(cockpitPre,kx,row,8),chan(cockpitPre,kx,row,0),
        chan(noFog,kx,row,16),chan(noFog,kx,row,8),chan(noFog,kx,row,0));

    // 1. Fog on everywhere: the unglassed right half is fogged in all fogged
    // renders and clearly different from the no-fog control.
    require(abs(lum(cockpitFix,kx,row) - lum(noFog,kx,row)) > 20, "fog applies on the control area (fixed)");
    require(abs(lum(cockpitPre,kx,row) - lum(noFog,kx,row)) > 20, "fog applies on the control area (pre-fix)");

    // 2. Bug reproduction: pre-fix, the window pixel matches the no-fog
    // control (glass depth blocks the fog), while the fixed render's window
    // pixel moves away from it by the fog contribution.
    require(abs(lum(cockpitPre,wx,row) - lum(noFog,wx,row)) <= 6, "pre-fix window shows no fog (bug reproduced)");
    require(abs(lum(cockpitFix,wx,row) - lum(noFog,wx,row)) > 15, "fixed window is fogged");

    // 3. Crack pane: visible through the glass once the glass stops writing
    // depth; occluded by the glass depth pre-fix (matches no-fog control,
    // where the pane is equally invisible).
    require(lum(cockpitFix,cx,row) > lum(cockpitPre,cx,row) + 40, "crack pane visible through the fixed window");
    require(abs(lum(cockpitPre,cx,row) - lum(noFog,cx,row)) <= 6, "pre-fix crack occluded (bug reproduced)");

    // 4. Chase unchanged: the chase render is pixel-identical to the pre-fix
    // cockpit render (same geometry, decision keeps depth writes on).
    {   int maxDelta = 0;
        for (int i = 0; i < W * H; i++)
            for (int s = 0; s < 3; s++) {
                int d = abs((int)((chaseFix[i] >> (s * 8)) & 0xff) - (int)((cockpitPre[i] >> (s * 8)) & 0xff));
                if (d > maxDelta) maxDelta = d;
            }
        printf("chase-vs-prefix max channel delta %d\n", maxDelta);
        require(maxDelta <= 1, "chase view unchanged by the gate");
    }

    // Same type 0x14, but this quad represents the opaque interior root.
    // Old classification removes its depth and beams paint over it. Fixed
    // ancestry preserves the shell depth while the open road still lights.
    static DWORD shellOldOff[W*H],shellOldOn[W*H],shellFixOff[W*H],shellFixOn[W*H];
    drawScene(shellOldOff,3,1,1,1,false);drawScene(shellOldOn,3,1,1,1,true);
    drawScene(shellFixOff,3,1,1,2,false);drawScene(shellFixOn,3,1,1,2,true);
    require(lum(shellOldOn,20,32)>lum(shellOldOff,20,32)+10,"old type-only gate allows beam through opaque shell");
    require(lum(shellFixOn,20,32)==lum(shellFixOff,20,32),"interior ancestry keeps shell beam-occluding");
    require(lum(shellFixOn,52,32)>lum(shellFixOff,52,32)+5,"open road still receives beam, not disabled");
    dumpPpm(prefix,"shell-old-beam",shellOldOn);dumpPpm(prefix,"shell-fixed-beam",shellFixOn);
    printf("cockpit_window_render ok (including opaque interior shell beam occlusion)\n");
    return 0;
}
