// Snow material (video.snow_material) rendered-pixel regression: quads with
// the lifted per-vertex diffuse colours drawn through the REAL shared 3D path
// (gfx_core recording -> real GL executor -> fixed3d pipeline) and read back
// as pixels. The colours come from the same SnowMaterial.h inlines the game
// loop calls, so the test exercises the actual channel the feature uses.
// Linux/software GL execution is NOT native Mac OpenGL acceptance; SDL_GPU
// (Metal) shades the same fixed3d source (pinned by snow_material_wiring).
#include "../src/render/gfx_core.cpp"
#include "../src/render/gfx_gl.cpp"
#include "../src/diagnostics/observer.cpp"
#include "SnowMaterial.h"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

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

static void putQuad(const DWORD diffuse[4])
{
    GfxVertex *v = Gfx_LockVertexBuffer(vb);
    require(v != NULL, "lock");
    const float px[4] = { -1.f, 1.f, -1.f, 1.f };
    const float py[4] = { -1.f, -1.f, 1.f, 1.f };
    for (int i = 0; i < 4; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].x = px[i]; v[i].y = py[i]; v[i].z = 1.f;
        v[i].nz = -1.f;
        v[i].diffuse = diffuse[i];
        v[i].specular = 0xff000000;
    }
    Gfx_UnlockVertexBuffer(vb);
}

static void drawAndRead(DWORD *pixels)
{
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f);
    Gfx_SetTexture(0, whiteTex);
    Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    DWORD dummy[4];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);   // marks the request
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}

static int chan(DWORD px, int c) { return (int)((px >> (c * 8)) & 0xff); }   // c=0,1,2 are the colour bytes, c=3 is alpha (ARGB)
static void colourBytes(DWORD px, int out[3])
{
    out[0] = chan(px, 0); out[1] = chan(px, 1); out[2] = chan(px, 2);
    for (int i = 0; i < 2; i++) for (int j = i + 1; j < 3; j++) if (out[j] < out[i]) { int t = out[i]; out[i] = out[j]; out[j] = t; }
}

static void dumpPpm(const char *prefix, const char *tag, const DWORD *pixels)
{
    if (!prefix) return;
    FILE *f = fopen((std::string(prefix) + "-" + tag + ".ppm").c_str(), "wb");
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int i = 0; i < W * H; i++) { int c[3]; colourBytes(pixels[i], c); fputc(c[0], f); fputc(c[1], f); fputc(c[2], f); }
    fclose(f);
}

int main(int argc, char **argv)
{
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    Gfx_PrepareGlWindow();
    testWindow = SDL_CreateWindow("snow material rendered pixels", W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    require(testWindow != NULL, SDL_GetError());
    require(Gfx_SetVideoMode(W, H, FALSE), "video mode");

    GfxViewport vp = { 0, 0, W, H, 0.f, 1.f };
    Gfx_SetViewport(&vp);
    GfxMatrix identity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &identity);
    Gfx_SetTransform(GFX_TRANSFORM_VIEW, &identity);
    // Ortho: x,y in -1..1, z in 0.5..2 -> clip.
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

    const char *prefix = argc > 1 ? argv[1] : nullptr;
    static DWORD frame[W * H];
    const DWORD BASE = 0xff808080;

    // 1. Baseline: no lift.
    DWORD grey[4] = { BASE, BASE, BASE, BASE };
    putQuad(grey);
    drawAndRead(frame);
    dumpPpm(prefix, "baseline", frame);
    int base[3]; colourBytes(frame[(H / 2) * W + W / 2], base);
    require(base[0] == base[2] && base[0] >= 120 && base[0] <= 136, "baseline grey on screen");

    // The real lift maths for a sun-aligned vertex (normal bisects sun and
    // view, centred facet jitter): response via the shared inlines.
    const unsigned CENTER = 127u | (127u << 8);
    float spec = SnowGlint_Specular(0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, CENTER);
    float response = SnowMaterial_Response(spec, 1.f, 0.f);
    DWORD lifted = SnowMaterial_LiftedDiffuse(BASE, response, SNOW_MATERIAL_LIFT_MAX * 1.f);
    require(lifted != BASE, "lift changes the vertex colour");

    // 2. Sun-facing snow: brighter by the computed response.
    DWORD sun[4] = { lifted, lifted, lifted, lifted };
    putQuad(sun);
    drawAndRead(frame);
    dumpPpm(prefix, "sun-facing", frame);
    int got[3]; colourBytes(frame[(H / 2) * W + W / 2], got);
    int expected = base[0] + (int)(response * SNOW_MATERIAL_LIFT_MAX);
    require(got[0] == got[2], "lift is white (channels rise equally)");
    require(got[0] >= expected - 2 && got[0] <= expected + 2, "pixel lift matches the computed response");

    // 3. Car-beam boost: response saturates at 1, lift hits the peak.
    DWORD beam = SnowMaterial_LiftedDiffuse(BASE, SnowMaterial_Response(0.f, 1.f, 1.f), SNOW_MATERIAL_LIFT_MAX * 1.f);
    DWORD beamQuad[4] = { beam, beam, beam, beam };
    putQuad(beamQuad);
    drawAndRead(frame);
    dumpPpm(prefix, "beam", frame);
    colourBytes(frame[(H / 2) * W + W / 2], got);
    // response = min(1, 0.8 * boost) = 0.8 at boost 1 -> lift = 0.8 * peak
    require(got[0] >= base[0] + (int)(0.8f * SNOW_MATERIAL_LIFT_MAX) - 3, "beam boost lifts toward peak");

    // 4. Option off (scale 0): exact identity.
    DWORD offC = SnowMaterial_LiftedDiffuse(BASE, 1.f, 0.f);
    require(offC == BASE, "scale 0 is the identity");
    DWORD off[4] = { offC, offC, offC, offC };
    putQuad(off);
    drawAndRead(frame);
    require(memcmp(frame, grey, 0) == 0 || true, "noop");
    int offPx[3]; colourBytes(frame[(H / 2) * W + W / 2], offPx);
    require(offPx[0] == base[0], "option off renders the original colour");

    // 5. View facing away from the sun: no specular, no lift.
    float away = SnowGlint_Specular(0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, -1.f, 0.f, CENTER);
    DWORD awayC = SnowMaterial_LiftedDiffuse(BASE, SnowMaterial_Response(away, 1.f, 0.f), SNOW_MATERIAL_LIFT_MAX * 1.f);
    require(awayC == BASE, "sun behind the view gives no lift");

    // 6. Shared-edge guard rendered: left half lifted, right half original
    // (the right vertices are the shared/non-snow side - they must not glow).
    DWORD half[4] = { lifted, BASE, lifted, BASE };
    putQuad(half);
    drawAndRead(frame);
    dumpPpm(prefix, "shared-edge", frame);
    int left[3], right[3];
    colourBytes(frame[(H / 2) * W + 4], left);
    colourBytes(frame[(H / 2) * W + W - 5], right);
    // A few pixels in from each edge: the snow side reaches the lifted colour,
    // the non-snow side stays within a sixth of the lift span of the original
    // (barycentric interpolation across the quad, not vertex bleeding).
    require(left[0] >= expected - 6, "snow side glows");
    require(right[0] >= base[0] - 2 && right[0] <= base[0] + (expected - base[0]) / 6,
            "non-snow side keeps the original colour");

    // 7. Restore: originals written back render the baseline exactly.
    putQuad(grey);
    drawAndRead(frame);
    dumpPpm(prefix, "restored", frame);
    colourBytes(frame[(H / 2) * W + W / 2], got);
    require(got[0] == base[0], "restored colours render the baseline");

    // 8. Two-triangle neighbour: one screen split into tri A (snow, lower-left
    // half) and tri B (non-snow, upper-right half) sharing the diagonal
    // vertices 1 and 2. The aggregation guard leaves shared vertices
    // unlifted: only vertex 0 (snow-only) is lifted. The whole non-snow
    // triangle - shared edge included - must render the original colour.
    {
        static const WORD idx[6] = { 0, 1, 2, 1, 3, 2 };
        DWORD neigh[4] = { lifted, BASE, BASE, BASE };   // v0 snow-only; v1,v2 shared; v3 non-snow
        putQuad(neigh);
        Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f);
        Gfx_SetTexture(0, whiteTex);
        Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, vb, 0, 4, idx, 6);
        DWORD dummy[4];
        (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);
        Gfx_Present(0);
        require(Gfx_ReadBackBuffer(0, 0, W, H, frame), "neighbour readback");
        dumpPpm(prefix, "neighbor", frame);
        int inside[3], edge[3], corner[3];
        colourBytes(frame[(H / 4) * W + (3 * W / 4)], inside);        // inside tri B
        colourBytes(frame[(H / 2) * W + (W / 2)], edge);              // on the shared diagonal
        colourBytes(frame[(3 * H / 4) * W + 2], corner);              // near vertex 0 (NDC y up)
        require(inside[0] >= base[0] - 2 && inside[0] <= base[0] + 2,
                "non-snow triangle keeps the original colour inside");
        require(edge[0] >= base[0] - 2 && edge[0] <= base[0] + 2,
                "shared edge keeps the original colour");
        require(corner[0] >= base[0] + (expected - base[0]) / 2,
                "snow triangle still glows at its own vertex");
    }

    printf("snow_material_render: ok\n");
    return 0;
}
