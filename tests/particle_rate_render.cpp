// Wheel spray / exhaust smoke rate-dial rendered evidence: the #226 dials
// change only HOW OFTEN the original emitters fire, so this simulates the
// emission pattern (positions over fixed 25 Hz steps, counts from the REAL
// ParticleAmount.h inlines the game calls) and draws the accumulated live
// particles as small alpha-blended quads through the REAL shared 3D path
// (gfx_core recording -> real GL executor -> fixed3d pipeline), then reads
// pixels back. Particle motion here is simplified (constant drift + fade),
// and a white texture substitutes for the original smoke/spray sprites: the
// test proves the SPATIAL DENSITY consequence of the rate change, not the
// shipped look. Linux/software GL execution is NOT native Mac acceptance.
#include "../src/render/gfx_core.cpp"
#include "../src/render/gfx_gl.cpp"
#include "../src/diagnostics/observer.cpp"
#include "ParticleAmount.h"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>

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
extern "C" double Sys_GetTicksPrecise(void) { return 0; }
extern "C" void Sys_GetDesktopSize(int *w, int *h) { *w = 96; *h = 64; }

static void require(bool ok, const char *what) { if (!ok) { fprintf(stderr,"FAIL: %s\n",what); exit(1); } }

static const int W = 96, H = 64;
static GfxVertexBuffer *vb;
static GfxTexture *whiteTex;

struct SimParticle { float x, y, vx, vy; int age, life; };

// Exhaust: one emitter at the car rear; each step spawns PuffCount(amount)
// puffs with a small velocity fan, drifting up-back over a 60-step life.
static std::vector<SimParticle> simExhaust(int amount, int steps)
{
    std::vector<SimParticle> live;
    unsigned int lcg = 777;
    for (int s = 0; s < steps; s++) {
        const int puffs = ParticleAmount_PuffCount(amount);
        for (int p = 0; p < puffs; p++) {
            lcg = lcg * 1103515245u + 12345u; const float jx = (((lcg >> 16) & 0xffff) / 65535.f - 0.5f) * 0.02f;
            lcg = lcg * 1103515245u + 12345u; const float jy = (((lcg >> 16) & 0xffff) / 65535.f) * 0.01f;
            live.push_back({ 0.55f, -0.35f, -0.006f + jx, 0.012f + jy, 0, 60 });
        }
        for (auto &q : live) { q.x += q.vx; q.y += q.vy; q.age++; }
        for (size_t i = 0; i < live.size();) if (live[i].age >= live[i].life) live[i] = live.back(), live.pop_back(); else i++;
    }
    return live;
}

// Spray: four wheel emitters on a countdown (original interval 4 steps);
// ScaledInterval(amount) shortens it; droplets arc down-out over a 24-step life.
static std::vector<SimParticle> simSpray(int amount, int steps)
{
    std::vector<SimParticle> live;
    const float wheels[4][2] = { {-0.5f, -0.45f}, {-0.2f, -0.45f}, {-0.5f, -0.1f}, {-0.2f, -0.1f} };
    int countdown[4] = { 1, 2, 3, 4 };
    unsigned int lcg = 4242;
    for (int s = 0; s < steps; s++) {
        for (int w = 0; w < 4; w++) {
            if (--countdown[w] > 0) continue;
            countdown[w] = ParticleAmount_ScaledInterval(4, amount);
            lcg = lcg * 1103515245u + 12345u; const float jx = (((lcg >> 16) & 0xffff) / 65535.f - 0.5f) * 0.01f;
            live.push_back({ wheels[w][0], wheels[w][1], -0.008f + jx, 0.004f, 0, 24 });
        }
        for (auto &q : live) { q.x += q.vx; q.y += q.vy; q.age++; }
        for (size_t i = 0; i < live.size();) if (live[i].age >= live[i].life) live[i] = live.back(), live.pop_back(); else i++;
    }
    return live;
}

static DWORD argb(int r, int g, int b, int a)
{
    return ((DWORD)a << 24) | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
}

static void drawParticles(const std::vector<SimParticle> &live, DWORD diffuse, DWORD *pixels)
{
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f);
    Gfx_SetTexture(0, whiteTex);
    const float half = 0.035f;
    for (const auto &p : live) {
        // Alpha fades with age, like the original smoke ramp.
        const int a = (int)((diffuse >> 24) * (p.life - p.age) / p.life);
        const DWORD d = (diffuse & 0x00ffffff) | ((DWORD)a << 24);
        GfxVertex *v = Gfx_LockVertexBuffer(vb);
        require(v != NULL, "lock");
        const float px[4] = { p.x - half, p.x + half, p.x - half, p.x + half };
        const float py[4] = { p.y - half, p.y - half, p.y + half, p.y + half };
        for (int i = 0; i < 4; i++) {
            memset(&v[i], 0, sizeof v[i]);
            v[i].x = px[i]; v[i].y = py[i]; v[i].z = 1.f;
            v[i].nz = -1.f;
            v[i].diffuse = d;
            v[i].specular = 0xff000000;
        }
        Gfx_UnlockVertexBuffer(vb);
        Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    }
    DWORD dummy[4];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}

static int chan(DWORD px, int c) { return (int)((px >> (c * 8)) & 0xff); }
static double meanLift(const DWORD *pixels)
{
    double s = 0;
    for (int i = 0; i < W * H; i++) s += chan(pixels[i], 0) + chan(pixels[i], 1) + chan(pixels[i], 2);
    return s / (W * H);
}
static double covered(const DWORD *pixels)
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
    for (int i = 0; i < W * H; i++) { fputc(chan(pixels[i],2),f); fputc(chan(pixels[i],1),f); fputc(chan(pixels[i],0),f); }
    fclose(f);
}

int main(int argc, char **argv)
{
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    Gfx_PrepareGlWindow();
    testWindow = SDL_CreateWindow("particle rate rendered pixels", W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
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
    Gfx_SetDepthTest(FALSE);
    Gfx_SetDepthWrite(FALSE);
    Gfx_SetAlphaBlend(TRUE);
    Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);
    Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetStageAlphaOp(0, GFX_TOP_MODULATE);
    Gfx_SetAlphaTest(FALSE);

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
    const DWORD smoke = argb(150, 150, 150, 110);   // grey smoke substitute
    const DWORD spray = argb(190, 200, 210, 130);   // pale spray substitute

    // Exhaust: live count at 200% must be about double the original 100%.
    {   auto live100 = simExhaust(100, 120);
        auto live200 = simExhaust(200, 120);
        drawParticles(live100, smoke, frame);
        const double liftA = meanLift(frame), covA = covered(frame);
        dumpPpm(prefix, "exhaust-100", frame);
        drawParticles(live200, smoke, frame);
        const double liftB = meanLift(frame), covB = covered(frame);
        dumpPpm(prefix, "exhaust-200", frame);
        require(live200.size() > live100.size() * 3 / 2, "200% exhaust keeps roughly double the live puffs");
        require(liftB > liftA + 1.0, "200% exhaust cloud is denser on screen");
        require(covB > covA + 0.005, "200% exhaust cloud covers more screen");
        printf("exhaust: 100%% live %zu lift %.1f cov %.2f; 200%% live %zu lift %.1f cov %.2f\n",
               live100.size(), liftA, covA, live200.size(), liftB, covB);
    }

    // Spray: 200% fires every 2 steps instead of every 4.
    {   auto live100 = simSpray(100, 96);
        auto live200 = simSpray(200, 96);
        drawParticles(live100, spray, frame);
        const double liftA = meanLift(frame), covA = covered(frame);
        dumpPpm(prefix, "spray-100", frame);
        drawParticles(live200, spray, frame);
        const double liftB = meanLift(frame), covB = covered(frame);
        dumpPpm(prefix, "spray-200", frame);
        require(live200.size() > live100.size() * 3 / 2, "200% spray keeps roughly double the live droplets");
        require(liftB > liftA + 1.0, "200% spray cloud is denser on screen");
        require(covB >= covA - 0.01, "200% spray coverage does not shrink");
        printf("spray: 100%% live %zu lift %.1f cov %.2f; 200%% live %zu lift %.1f cov %.2f\n",
               live100.size(), liftA, covA, live200.size(), liftB, covB);
    }

    // Sampled draw cost of the doubled load (sandbox software GL; relative only, NOT native perf).
    {   static DWORD sink[W * H];
        auto live100 = simExhaust(100, 120), live200 = simExhaust(200, 120);
        for (int i = 0; i < 5; i++) drawParticles(live100, smoke, sink);   // warm-up: first draws pay pipeline setup
        unsigned int s0 = SDL_GetTicks();
        for (int i = 0; i < 50; i++) drawParticles(live100, smoke, sink);
        unsigned int s1 = SDL_GetTicks();
        for (int i = 0; i < 50; i++) drawParticles(live200, smoke, sink);
        unsigned int s2 = SDL_GetTicks();
        printf("perf sample (sandbox software GL, relative only): %zu quads %.2f ms/frame, %zu quads %.2f ms/frame\n",
               live100.size(), (s1 - s0) / 50.0, live200.size(), (s2 - s1) / 50.0);
    }

    printf("particle_rate_render: ok\n");
    return 0;
}
