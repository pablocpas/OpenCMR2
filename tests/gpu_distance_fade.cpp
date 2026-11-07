// Exercise distance fade snapshots on the real GPU, without game assets.
// A display and a Vulkan/Metal/D3D12 device are required (software Vulkan works).
#include "platform/platform.h"
#include "render/gfx_internal.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>

static SDL_Window *window;
SDL_Window *Platform_GetWindow() { return window; }
int Platform_GetSettingInt(const char *key, int fallback)
{
    return !strcmp(key, "video.gpu_debug") ? 1 : fallback;
}
const char *Platform_GetSetting(const char *key, const char *fallback)
{
    return !strcmp(key, "video.graphics_api") ? "sdl_gpu" : fallback;
}
extern "C" int Sys_GetOption(const char *, int fallback) { return fallback; }
extern "C" void Sys_GetDesktopSize(int *w, int *h) { *w = 32; *h = 16; }
extern "C" void Sys_Log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
}

static void Viewport(int x, int y, int w, int h)
{
    GfxViewport v = {DWORD(x), DWORD(y), DWORD(w), DWORD(h), 0.0f, 1.0f};
    Gfx_SetViewport(&v);
}

static void Draw3D(DWORD color, float depth = 0.75f)
{
    GfxVertex v[3] = {};
    v[0].x = -2; v[0].y = 2;
    v[1].x = 4; v[1].y = 2;
    v[2].x = -2; v[2].y = -4;
    for (auto &p : v) { p.z = depth; p.diffuse = color; }
    Gfx_DrawPrimitive(GFX_TRIANGLELIST, v, 3);
}

static void Hud()
{
    GfxTLVertex v[] = {
        {-0.5f, -0.5f, 0, 1, 0xffffffff, 0, 0, 0},
        {63.5f, -0.5f, 0, 1, 0xffffffff, 0, 0, 0},
        {-0.5f, 31.5f, 0, 1, 0xffffffff, 0, 0, 0},
    };
    Viewport(0, 0, 4, 4);
    Gfx_DrawPrimitiveTL(GFX_TRIANGLELIST, v, 3);
    Viewport(0, 0, 32, 16);
}

static bool Matches(DWORD color, DWORD expected)
{
    for (int shift = 0; shift < 32; shift += 8)
        if (abs(int((color >> shift) & 255) - int((expected >> shift) & 255)) > 1)
            return false;
    return true;
}

static DWORD Mix(DWORD a, DWORD b, int amount)
{
    DWORD result = 0xff000000;
    for (int shift = 0; shift < 24; shift += 8)
        result |= (((a >> shift & 255) * (2 - amount) + (b >> shift & 255) * amount + 1) / 2) << shift;
    return result;
}

int main()
{
    SDL_SetHint(SDL_HINT_ASSERT, "abort");
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "OPENCMR2_HEADLESS", "0", true);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "gpu distance fade: skipped: %s\n", SDL_GetError());
        SDL_Quit();
        return 77;
    }
    const char *driver = SDL_GetCurrentVideoDriver();
    if (!strcmp(driver, "dummy") || !strcmp(driver, "offscreen") ||
        !SDL_GPUSupportsShaderFormats(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, nullptr)) {
        fprintf(stderr, "gpu distance fade: skipped: no GPU window support (%s): %s\n", driver, SDL_GetError());
        SDL_Quit();
        return 77;
    }
    window = SDL_CreateWindow("OpenCMR2 GPU distance fade test", 32, 16, SDL_WINDOW_HIDDEN);
    if (!window || !Gfx_SetVideoMode(32, 16, FALSE)) {
        fprintf(stderr, "gpu distance fade: failed to create renderer: %s\n", SDL_GetError());
        return 1;
    }
    Gfx_SetLighting(FALSE);
    Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetStageColorOp(0, GFX_TOP_DISABLE);
    Gfx_SetStageColorOp(1, GFX_TOP_DISABLE);
    // Keep eye-space XY tiny while spanning the screen: every fragment's
    // distance is 0.75, allowing exact near/mid/far fade expectations.
    GfxMatrix world = {}, identity = {}, projection = {};
    identity._11 = identity._22 = identity._33 = identity._44 = 1;
    world = projection = identity;
    world._11 = world._22 = 0.0001f;
    projection._11 = projection._22 = 10000;
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &world);
    Gfx_SetTransform(GFX_TRANSFORM_VIEW, &identity);
    Gfx_SetTransform(GFX_TRANSFORM_PROJECTION, &projection);
    auto *target = Gfx_CreateTexture(4, 4, GFX_FORMAT_BGRA8, 1, GFX_TEXTURE_RENDER_TARGET);
    bool correct = true;
    unsigned checked = 0, reads = 0;
    for (unsigned frame = 0; correct && frame < 48; frame++) {
        if (frame && frame % 8 == 0) {
            // Unload/recreate a target while other frames may still use it.
            Gfx_DestroyTexture(target);
            target = Gfx_CreateTexture(4, 4, GFX_FORMAT_BGRA8, 1, GFX_TEXTURE_RENDER_TARGET);
        }
        const DWORD background = frame & 1 ? 0xff204060 : 0xff80c0e0;
        const DWORD foreground = frame & 2 ? 0xffc08020 : 0xff4080c0;
        const int fade = int(frame % 3); // 0, 1/2, 1
        Gfx_SetDistanceFade(0, 0);
        Gfx_SetDepthTest(TRUE);
        Gfx_SetDepthWrite(TRUE);
        Gfx_SetDepthFunc(GFX_CMP_LESSEQUAL);
        Viewport(0, 0, 32, 16);
        Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1);
        Draw3D(background);
        Gfx_CaptureBackdrop();
        if (fade == 0) Gfx_SetDistanceFade(1, 2);
        if (fade == 1) Gfx_SetDistanceFade(0.5f, 1);
        if (fade == 2) Gfx_SetDistanceFade(0.25f, 0.5f);
        // Render targets must ignore the fade even when it is active. Return
        // to the same world transforms and viewport before drawing scenery.
        if (frame % 2 == 0) {
            Gfx_SetRenderTarget(target);
            Viewport(0, 0, 4, 4);
            Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1);
            Draw3D(foreground);
            Gfx_SetRenderTarget(nullptr);
            Viewport(0, 0, 32, 16);
        }
        Draw3D(foreground);
        if (frame % 2 == 0) {
            // A second snapshot in one command buffer must not replace the
            // first one while preceding draws are still reading from it.
            Gfx_CaptureBackdrop();
            Gfx_SetDistanceFade(0.25f, 0.5f);
            Draw3D(0xff000000);
        }
        Gfx_SetDistanceFade(0, 0);
        // Both snapshots must preserve the original scene depth.
        Draw3D(0xffffffff, 0.9f);
        Gfx_SetDistanceFade(0.25f, 0.5f);
        Hud(); // transformed UI ignores the distance fade
        DWORD pixels[32 * 16];
        const bool read = frame % 4 == (frame / 4 % 2 ? 3 : 2);
        // A different extent rejects a completed read of an earlier frame.
        const int readWidth = 32 - int(reads);
        if (read) Gfx_ReadBackBuffer(0, 0, readWidth, 16, pixels);
        Gfx_Present(FALSE);
        if (!read) continue; // keep several frames in flight
        const Uint64 start = SDL_GetTicks();
        bool ready;
        while (!(ready = Gfx_ReadBackBuffer(0, 0, readWidth, 16, pixels)) && SDL_GetTicks() - start < 5000)
            SDL_Delay(1);
        if (!ready) { fprintf(stderr, "gpu distance fade: readback timeout\n"); correct = false; break; }
        ++reads;
        const DWORD scenery = Mix(foreground, background, fade);
        for (int y = 0; correct && y < 16; y++)
            for (int x = 0; x < readWidth; x++) {
                const DWORD expected = x < 4 && y < 4 ? 0xffffffff : scenery;
                if (!Matches(pixels[y * readWidth + x], expected)) {
                    fprintf(stderr, "gpu distance fade: frame %u pixel %d,%d: %08x, expected %08x\n",
                            frame, x, y, unsigned(pixels[y * readWidth + x]), unsigned(expected));
                    correct = false;
                    break;
                }
                checked++;
            }
        if (frame % 2 != 0) continue;
        GfxLockedRect rect;
        if (!Gfx_LockTexture(target, 0, 0, &rect)) { correct = false; break; }
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++)
                if (!Matches(((DWORD *)((BYTE *)rect.pixels + y * rect.pitch))[x], foreground)) correct = false;
        Gfx_UnlockTexture(target, 0, 0);
    }
    Gfx_DestroyTexture(target);
    Gfx_Shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (!correct) return 1;
    printf("gpu distance fade: %u pixels; near/mid/far, HUD, targets, repeated captures and resource reloads\n", checked);
    return 0;
}
