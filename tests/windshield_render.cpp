// Windshield dirt pane rendered-pixel regression (video.windscreen_dirt):
// the REAL production draw (game/WindshieldDirt.cpp: Windshield_DrawCrackPane
// -> the dirt pane) runs over a real dirt map through gfx_core -> the real GL
// executor with readback. The glass mesh itself (game/WindshieldGlass.cpp)
// needs the game's car data, so this harness stands in a trapezoid glass
// polygon with glass-local uv: dirt must appear on it and nowhere else. Linux/software GL is NOT native Mac OpenGL/Metal
// acceptance; the SDL_GPU path draws the same TL command stream.
#include "../src/render/gfx_core.cpp"
#include "../src/render/gfx_gl.cpp"
#include "../src/diagnostics/observer.cpp"
#include "../game/WindshieldDirt.cpp"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

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

int WindshieldGlass_DrawCockpitExtension(int) { return 0; }
int WindshieldGlass_GetInfo(int,int*,int*,unsigned*,int*,int*) {return 0;}
// Stand-in glass: a trapezoid in the upper part of the frame, uv = position
// inside its bounds (u across, v down), like WsGlassFrame_UV on a real glass.
static const float kGlassTop = 0.8f, kGlassBottom = -0.4f, kGlassL = -0.8f, kGlassR = 0.8f;
static const float kAspect = 0.42f;
int WindshieldGlass_GetFrontFrame(int, WsGlassFrame *out) {
    out->valid = 1; out->zMin = kGlassL; out->zMax = kGlassR; out->yMin = kGlassBottom; out->yMax = kGlassTop;
    out->widthUnits = kGlassR - kGlassL; out->heightUnits = (kGlassTop - kGlassBottom);
    out->aspect = kAspect;
    return 1;
}
int WindshieldGlass_DrawDirtPane(int, GfxTexture *tex) {
    // Trapezoid: the top edge is narrower than the bottom (raked pillars).
    static const float inset = 0.2f;
    GfxVertex gv[6];
    const float px[6] = { kGlassL + inset, kGlassR - inset, kGlassL,   kGlassR - inset, kGlassR, kGlassL };
    const float py[6] = { kGlassTop,       kGlassTop,       kGlassBottom, kGlassTop,     kGlassBottom, kGlassBottom };
    for (int i = 0; i < 6; i++) {
        memset(&gv[i], 0, sizeof gv[i]);
        gv[i].x = px[i]; gv[i].y = py[i]; gv[i].z = 1.0f;
        gv[i].nz = -1.f; gv[i].diffuse = 0xffffffff;
        gv[i].u0 = (px[i] - kGlassL) / (kGlassR - kGlassL);
        gv[i].v0 = (kGlassTop - py[i]) / (kGlassTop - kGlassBottom);
    }
    Gfx_PushState();
    Gfx_SetLighting(FALSE); Gfx_SetFog(FALSE); Gfx_SetAlphaTest(FALSE);
    Gfx_SetDepthTest(TRUE); Gfx_SetDepthWrite(FALSE); Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetAlphaBlend(TRUE); Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA); Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetStageColorOp(0, GFX_TOP_MODULATE); Gfx_SetStageColorArg1(0, GFX_TA_TEXTURE); Gfx_SetStageColorArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageAlphaOp(0, GFX_TOP_SELECTARG1); Gfx_SetStageAlphaArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageColorOp(1, GFX_TOP_DISABLE); Gfx_SetStageAlphaOp(1, GFX_TOP_DISABLE);
    Gfx_SetStageAddress(0, GFX_ADDRESS_CLAMP);
    Gfx_SetTexture(0, tex);
    Gfx_DrawPrimitive(GFX_TRIANGLELIST, gv, 6);
    Gfx_SetTexture(0, NULL);
    Gfx_PopState();
    return 1;
}
CarPartSet *StageTiming_GetCarReplayRecord(int) {return nullptr;}
void TouchOverlay_RecordFrame(int,int,bool) {}
int WindshieldGlass_ApplyDamageState(int,int) {return 0;}
int WindshieldGlass_DrawSidePanes(int,int,int) {return 0;}
int WindshieldGlass_DrawCrackPane(int,int) {return 0;}

// Stubs for WindshieldDirt.cpp's game-side externs (unused by the draw path).
StageBlock g_stageBlock;
int StageObject_GetViewWeatherField54(int) { return 0; }

Graphics *g_pGraphics;
double CGraphics::m_oneOver65536 = 1.0 / 65536.0;
double CGraphics::m_65536 = 65536.0;
static D3DTextureManager g_texManager;
D3DTextureManager *CGraphics::m_pTextureManager = &g_texManager;
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
        v[i].x = px[i]; v[i].y = py[i]; v[i].z = 1.99f;
        v[i].nz = -1.f;
        v[i].diffuse = i < 2 ? 0xff4a4038 : 0xff90a8c8;   // dark road below, bright sky above (ARGB)
        v[i].specular = 0xff000000;
    }
    Gfx_UnlockVertexBuffer(vb);
}

static void drawAndRead(DWORD *pixels, bool withOverlay, bool poisoned = false)
{
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff181c20, 1.0f);
    Gfx_SetTexture(0, whiteTex);
    Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    if (withOverlay) {
        Gfx_PushState();
        if (poisoned) {
            Gfx_SetStageColorOp(0, GFX_TOP_DISABLE);
            Gfx_SetAlphaTest(TRUE);Gfx_SetAlphaFunc(GFX_CMP_NEVER);
            Gfx_SetFog(TRUE);Gfx_SetFogColor(0xffffffff);
            Gfx_SetCullMode(GFX_CULL_CW);
        }
        Windshield_DrawCrackPane(0);   // the production 3D-pass call: crack pane + dirt pane
        Gfx_PopState();
    }
    Gfx_EndWorldEffects();
    DWORD dummy[4];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}

// Pixel of a glass-local uv on the stand-in glass (the trapezoid's bounding box).
static int glassPx(float u) { return (int)((kGlassL + u * (kGlassR - kGlassL) + 1.f) * 0.5f * W); }
static int glassPy(float v) { return (int)((1.f - (kGlassTop - v * (kGlassTop - kGlassBottom))) * 0.5f * H); }

static int chan(const DWORD *f, int x, int y, int shift) { return (int)((f[y * W + x] >> shift) & 0xff); }

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
    testWindow = SDL_CreateWindow("windshield overlay rendered pixels", W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
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

    vb = Gfx_CreateVertexBuffer(4);
    require(vb != NULL, "vertex buffer");
    whiteTex = Gfx_CreateTexture(1, 1, GFX_FORMAT_BGRA8, 1, 0);
    require(whiteTex != NULL, "white texture");
    {   GfxLockedRect lr;
        require(Gfx_LockTexture(whiteTex, 0, 0, &lr), "lock white");
        memset(lr.pixels, 0xff, 4);
        Gfx_UnlockTexture(whiteTex, 0, 0);
    }

    // EXTREME FIXTURE (boundary test, not a representative look): saturated
    // dirt over the whole glass, one dry blade arc wiped through it.
    Windshield_ResetAll();
    WindshieldMap *m = &g_windshieldMaps[0];
    for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++)
        m->dirt[i] = Windshield_InsideGlass((i % WINDSHIELD_MAP_W + 0.5f) / WINDSHIELD_MAP_W,
                                            (i / WINDSHIELD_MAP_W + 0.5f) / WINDSHIELD_MAP_H) ? 220 : 0;
    m->wet = 0;
    require(Windshield_Wipe(m, 0.34f, 1.02f, 0.72f, -0.25f, 0.06f, 1) > 0, "wipe removes dirt");

    // REPRESENTATIVE look: the production accumulation path
    // (Windshield_AddSpatter, as driving calls it) at a moderate level, then
    // two wet blade sweeps at different angles - the readable everyday state.
    WindshieldMap *r = &g_windshieldMaps[1];
    Windshield_Reset(r, 0x5eedu);
    for (int pass = 0; pass < 80; pass++)   // ~80 driving spatter events
        Windshield_AddSpatter(r, 3, 100, 0.3f);
    for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++)
        if (r->dirt[i] > 150) r->dirt[i] = 150;   // cap at a mid accumulation level
    r->wet = 200;
    require(Windshield_Wipe(r, 0.34f, 1.02f, 0.72f, -0.25f, 0.06f, 0) > 0, "wet sweep one");
    require(Windshield_Wipe(r, 0.34f, 1.02f, 0.72f, 0.35f, 0.06f, 0) > 0, "wet sweep two");

    const char *prefix = argc > 1 ? argv[1] : nullptr;
    static DWORD clean[W * H], dirty[W * H], repr[W * H];
    // 1. Control: no dirt.
    WindshieldMap saved = *m;
    memset(m->dirt, 0, sizeof m->dirt);
    putQuad();
    drawAndRead(clean, true);
    // 2. Dirty glass with the wiped arc (extreme fixture).
    *m = saved;
    putQuad();
    drawAndRead(dirty, true);
    // 3. Representative: moderate production spatter + two wet sweeps.
    {   WindshieldMap keep = *m;
        *m = *r;
        putQuad();
        drawAndRead(repr, true);
        *m = keep;
    }

    static DWORD isolated[W*H];
    *m = saved;
    putQuad();
    drawAndRead(isolated, true, true);
    require(memcmp(isolated, dirty, sizeof dirty) == 0, "dirt pane ignores inherited alpha/stage/fog/cull state");
    dumpPpm(prefix, "clean", clean);
    dumpPpm(prefix, "dirty", dirty);
    dumpPpm(prefix, "representative", repr);
    // The glass centre, away from the wiped arc, darkens toward the grime tint.
    require(chan(dirty,glassPx(0.5f),glassPy(0.2f),0) < chan(clean,glassPx(0.5f),glassPy(0.2f),0) - 24, "dirt darkens the glass");
    // The wiped arc stays at the clean level: blade pivot (0.34,1.02), angle -0.25 -> cell u=0.216, v=0.536.
    require(chan(dirty,glassPx(0.216f),glassPy(0.536f),16) >= chan(clean,glassPx(0.216f),glassPy(0.536f),16) - 8, "wiped arc stays clean");
    // Nothing outside the glass polygon changes: below the glass (the dash), and
    // the trapezoid's cut-off top corners, even though the map has dirt there.
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float nx = (x + 0.5f) / W * 2.f - 1.f, ny = 1.f - (y + 0.5f) / H * 2.f;
            bool outside = ny > kGlassTop + 0.03f || ny < kGlassBottom - 0.03f || nx < kGlassL - 0.03f || nx > kGlassR + 0.03f;
            if (outside) require(dirty[y * W + x] == clean[y * W + x], "no dirt outside the glass");
        }
    // Top-left corner cut by the raked pillar: glass bounds contain it, the polygon does not.
    require(dirty[glassPy(0.03f) * W + glassPx(0.02f)] == clean[glassPy(0.03f) * W + glassPx(0.02f)], "no dirt on the cut-off corner");
    // Representative: sky through partial dirt stays readable (much closer to
    // clean than the saturated fixture), and both swept arcs sit between.
    require(chan(repr,glassPx(0.5f),glassPy(0.2f),0) > chan(dirty,glassPx(0.5f),glassPy(0.2f),0) + 24, "representative dirt stays readable");
    {   int maxDark = 0;
        for (int y = 4; y < 56; y++)
            for (int x = 8; x < 88; x++) {
                int d = chan(clean,x,y,0) - chan(repr,x,y,0);
                if (d > maxDark) maxDark = d;
            }
        require(maxDark >= 24, "representative dirt is visible");
        printf("representative max darkening %d\n", maxDark);
    }
    printf("windshield_render ok\n");
    return 0;
}
