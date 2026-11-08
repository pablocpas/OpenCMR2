// Dust/snow plume rendered-pixel harness: the amount dial scales how many
// live plume quads the emitter keeps in the air, so this draws that
// representative configuration -- a cloud of alpha-blended, per-vertex
// tinted quads through the REAL shared 3D path (gfx_core recording -> real
// GL executor -> fixed3d pipeline) -- at the original live cap (96) and at
// the 200% default cap from DustPlume_LiveCap, reads pixels back, and pins
// that the denser cloud covers more screen and lifts it further. The soil
// tint comes from the same DustPlume_SoilTint inline the game calls.
// The white texture substitutes for the cloud art asset; this proves the
// amount/tint/blend mechanism, not the shipped sprite. Linux/software GL
// execution is NOT native Mac OpenGL acceptance.
#include "../src/render/gfx_core.cpp"
#include "../src/render/gfx_gl.cpp"
#include "../src/diagnostics/observer.cpp"
#include "DustPlume.h"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cmath>

static SDL_Window *testWindow;
extern "C" int Sys_GetOption(const char *, int fallback) { return fallback; }
SDL_Window *Platform_GetWindow() { return testWindow; }
int Platform_GetSettingInt(const char *, int fallback) { return fallback; }
void TouchOverlay_RecordFrame(int, int, bool) {}
const char *Platform_GetSetting(const char *key, const char *fallback) {
    return !strcmp(key, "video.graphics_api") ? "opengl" : fallback;
}
extern "C" void Sys_Log(const char *format, ...) { va_list a; va_start(a,format); vfprintf(stderr,format,a); fputc('\n',stderr); va_end(a); }
GfxExecutor *Gfx_CreateGpuExecutor() { return nullptr; }
bool Gfx_GpuIsAvailable(void) { return false; }
extern "C" unsigned int Sys_GetTicks() { return 0; }
static double fakeNow = 0.0;
extern "C" double Sys_GetTicksPrecise(void) { return fakeNow; }
extern "C" void Sys_GetDesktopSize(int *w, int *h) { *w = 96; *h = 64; }

static void require(bool ok, const char *what) { if (!ok) { fprintf(stderr,"FAIL: %s\n",what); exit(1); } }

static const int W = 96, H = 64;
static GfxVertexBuffer *vb;
static GfxTexture *whiteTex;

static DWORD argb(int r, int g, int b, int a)
{
    return ((DWORD)a << 24) | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
}

// Draw `quads` alpha-blended quads on a deterministic grid (coverage grows
// with count) in one frame, then read the frame back.
static void drawCloud(int quads, DWORD diffuse, DWORD *pixels)
{
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f);
    Gfx_SetTexture(0, whiteTex);
    // Fixed-size quads jittered inside a cluster box: more quads stack more
    // overdraw, the way a denser live plume accumulates behind the car.
    unsigned int lcg = 12345;
    const float half = 0.22f;
    for (int q = 0; q < quads; q++) {
        lcg = lcg * 1103515245u + 12345u; const float jx = ((lcg >> 16) & 0xffff) / 65535.f;
        lcg = lcg * 1103515245u + 12345u; const float jy = ((lcg >> 16) & 0xffff) / 65535.f;
        const float cx = -0.6f + jx * 1.0f, cy = -0.6f + jy * 1.0f;
        const float x0 = cx - half, x1 = cx + half, y0 = cy - half, y1 = cy + half;
        GfxVertex *v = Gfx_LockVertexBuffer(vb);
        require(v != NULL, "lock");
        const float px[4] = { x0, x1, x0, x1 };
        const float py[4] = { y0, y0, y1, y1 };
        for (int i = 0; i < 4; i++) {
            memset(&v[i], 0, sizeof v[i]);
            v[i].x = px[i]; v[i].y = py[i]; v[i].z = 1.f;
            v[i].nz = -1.f;
            v[i].diffuse = diffuse;
            v[i].specular = 0xff000000;
        }
        Gfx_UnlockVertexBuffer(vb);
        Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    }
    DWORD dummy[4];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);   // marks the request
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}

// Readback DWORD is A,R,G,B from high to low byte: byte 0 = blue, byte 2 = red.
static int chan(DWORD px, int c) { return (int)((px >> (c * 8)) & 0xff); }
static int redOf(DWORD px) { return chan(px, 2); }
static int blueOf(DWORD px) { return chan(px, 0); }
static double meanLift(const DWORD *pixels)   // mean per-pixel channel sum over the black background
{
    double s = 0;
    for (int i = 0; i < W * H; i++) s += chan(pixels[i], 0) + chan(pixels[i], 1) + chan(pixels[i], 2);
    return s / (W * H);
}
static double covered(const DWORD *pixels)    // fraction of pixels lifted above the background
{
    int n = 0;
    for (int i = 0; i < W * H; i++) if (chan(pixels[i], 0) + chan(pixels[i], 1) + chan(pixels[i], 2) > 12) n++;
    return (double)n / (W * H);
}

static void dumpPpm(const char *prefix, const char *tag, const DWORD *pixels)
{
    if (!prefix) return;
    FILE *f = fopen((std::string(prefix) + "-" + tag + ".ppm").c_str(), "wb");
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int i = 0; i < W * H; i++) { fputc(redOf(pixels[i]),f); fputc(chan(pixels[i],1),f); fputc(blueOf(pixels[i]),f); }
    fclose(f);
}

int main(int argc, char **argv)
{
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    Gfx_PrepareGlWindow();
    testWindow = SDL_CreateWindow("dust plume rendered pixels", W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    require(testWindow != NULL, SDL_GetError());
    require(Gfx_SetVideoMode(W, H, FALSE), "video mode");

    GfxViewport vp = { 0, 0, W, H, 0.f, 1.f };
    Gfx_SetViewport(&vp);
    GfxMatrix identity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &identity);
    Gfx_SetTransform(GFX_TRANSFORM_VIEW, &identity);
    GfxMatrix proj = { 1,0,0,0, 0,1,0,0, 0,0,1.f/1.5f,0, 0,0,-0.5f/1.5f,1 };
    Gfx_SetTransform(GFX_TRANSFORM_PROJECTION, &proj);
    Gfx_SetLighting(FALSE);
    Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetDepthTest(FALSE);              // plume particles alpha-blend against the world without depth-fighting each other
    Gfx_SetDepthWrite(FALSE);
    Gfx_SetAlphaBlend(TRUE);
    Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);
    Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetStageAlphaOp(0, GFX_TOP_MODULATE);   // texture alpha * vertex alpha, as the game's particle state does
    Gfx_SetAlphaTest(TRUE);
    Gfx_SetAlphaFunc(GFX_CMP_GREATEREQUAL);
    Gfx_SetAlphaRef(1);

    vb = Gfx_CreateVertexBuffer(4);
    require(vb != NULL, "vertex buffer");
    whiteTex = Gfx_CreateTexture(1, 1, GFX_FORMAT_BGRA8, 1, 0);
    require(whiteTex != NULL, "white texture");
    {   GfxLockedRect lr;
        require(Gfx_LockTexture(whiteTex, 0, 0, &lr), "lock white");
        memset(lr.pixels, 0xff, 4);
        Gfx_UnlockTexture(whiteTex, 0, 0);
    }

    const char *prefix = argc > 1 ? argv[1] : nullptr;
    static DWORD frame[W * H];

    // Cap pins: the dial maps to live cloud caps through the same inline the emitter calls.
    require(DustPlume_LiveCap(100) == 96, "100% keeps the original 96 live cap");
    require(DustPlume_LiveCap(200) == 192, "200% default doubles the live cap");
    require(DustPlume_LiveCap(300) == 288, "300% triples the live cap");
    require(DustPlume_LiveCap(0) == 0, "0% emits no plume");

    // Dirt tint: green-dominant ground shifts warm (brown rooster-tail) through the real inline.
    {   float r = 0x40/255.f, g = 0x60/255.f, b = 0x30/255.f;
        DustPlume_SoilTint(r, g, b);
        require(r > g && g > b, "soil tint orders red over green over blue");
    }

    const int alpha = 60;   // mid-life plume alpha from the DUST_PLUME_LIFE fade curve
    // Dirt scenario at the original cap and at the 200% default cap.
    float dr = 0x40/255.f, dg = 0x60/255.f, db = 0x30/255.f;
    DustPlume_SoilTint(dr, dg, db);
    const DWORD dirt = argb((int)(dr*255), (int)(dg*255), (int)(db*255), alpha);
    drawCloud(96, dirt, frame);
    const double liftA = meanLift(frame), covA = covered(frame);
    dumpPpm(prefix, "dirt-original-96", frame);
    require(liftA > 1.0, "original dirt cloud lifts the frame");
    drawCloud(192, dirt, frame);
    const double liftB = meanLift(frame), covB = covered(frame);
    dumpPpm(prefix, "dirt-overdo-192", frame);
    require(liftB > liftA + 1.0, "200% cloud is denser than the original");
    require(covB >= covA - 0.01, "200% cloud covers at least the original area");
    {   // Overdo cloud stays dirt-tinted, not white: red leads blue.
        double rs = 0, bs = 0;
        for (int i = 0; i < W * H; i++) { rs += redOf(frame[i]); bs += blueOf(frame[i]); }
        require(rs > bs, "dirt cloud keeps the warm tint");
    }
    printf("dirt: 96 quads lift %.1f cov %.2f; 192 quads lift %.1f cov %.2f\n", liftA, covA, liftB, covB);

    // Snow scenario: bright neutral ground keeps its colour through the inline (no soil shift).
    {   float sr = 0xe8/255.f, sg = 0xe8/255.f, sb = 0xe0/255.f, or_ = sr, og = sg, ob = sb;
        DustPlume_SoilTint(sr, sg, sb);
        require(sr == or_ && sg == og && sb == ob, "snow ground keeps its bright neutral colour");
        const DWORD snow = argb((int)(sr*255), (int)(sg*255), (int)(sb*255), alpha);
        drawCloud(192, snow, frame);
        dumpPpm(prefix, "snow-overdo-192", frame);
        require(meanLift(frame) > liftB, "snow cloud renders brighter than the dirt cloud");
        printf("snow: 192 quads lift %.1f\n", meanLift(frame));
    }

    // Sampled draw cost at the original and maximum caps (sandbox software GL; relative only, NOT native perf).
    {   static DWORD sink[W * H];
        fakeNow = 0; double t0 = 0, t1 = 0, t2 = 0;
        fakeNow = 0; t0 = 0;
        // crude wall-clock sample via SDL ticks
        unsigned int s0 = SDL_GetTicks();
        for (int i = 0; i < 50; i++) drawCloud(96, dirt, sink);
        unsigned int s1 = SDL_GetTicks();
        for (int i = 0; i < 50; i++) drawCloud(288, dirt, sink);
        unsigned int s2 = SDL_GetTicks();
        t0 = (s1 - s0) / 50.0; t1 = (s2 - s1) / 50.0; t2 = t0;
        printf("perf sample (sandbox software GL, relative only): 96 quads %.2f ms/frame, 288 quads %.2f ms/frame\n", t2, t1);
    }

    printf("dust_plume_render: ok\n");
    return 0;
}
