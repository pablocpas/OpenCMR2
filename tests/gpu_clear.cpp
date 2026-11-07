// Exercise viewport clears on the real SDL_GPU backend, without game assets.
// A display and a Vulkan/Metal/D3D12 device are required (software Vulkan works).
#include "platform/platform.h"
#include "render/gfx_internal.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

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

static void Draw(float depth, DWORD color)
{
    GfxTLVertex v[] = {
        {-0.5f, -0.5f, depth, 1, color, 0, 0, 0},
        {63.5f, -0.5f, depth, 1, color, 0, 0, 0},
        {-0.5f, 31.5f, depth, 1, color, 0, 0, 0},
    };
    Gfx_DrawPrimitiveTL(GFX_TRIANGLELIST, v, 3);
}

int main()
{
    SDL_SetHint(SDL_HINT_ASSERT, "abort");
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "OPENCMR2_HEADLESS", "0", true);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "gpu clear: skipped: %s\n", SDL_GetError());
        SDL_Quit();
        return 77;
    }
    const char *driver = SDL_GetCurrentVideoDriver();
    if (!strcmp(driver, "dummy") || !strcmp(driver, "offscreen") ||
        !SDL_GPUSupportsShaderFormats(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, nullptr)) {
        fprintf(stderr, "gpu clear: skipped: no GPU window support (%s): %s\n", driver, SDL_GetError());
        SDL_Quit();
        return 77;
    }
    window = SDL_CreateWindow("OpenCMR2 GPU clear test", 32, 16, SDL_WINDOW_HIDDEN);
    if (!window || !Gfx_SetVideoMode(32, 16, FALSE)) {
        fprintf(stderr, "gpu clear: failed to create renderer: %s\n", SDL_GetError());
        return 1;
    }
    Gfx_SetLighting(FALSE);
    Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetDepthFunc(GFX_CMP_LESS);
    Gfx_SetStageColorOp(0, GFX_TOP_DISABLE);
    Gfx_SetStageColorOp(1, GFX_TOP_DISABLE);

    Viewport(0, 0, 32, 16);
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f);
    Draw(0.25f, 0xffff0000);
    // This clear occurs after a draw and covers only part of the target:
    // it must use the clear pipeline, rather than the pass's load operation.
    Viewport(8, 4, 16, 8);
    Gfx_Clear(GFX_CLEAR_ZBUFFER, 0, 1.0f);
    Viewport(0, 0, 32, 16);
    Draw(0.75f, 0xff00ff00);
    // Clearing colour must preserve the depth already written there.
    Viewport(0, 0, 8, 16);
    Gfx_Clear(GFX_CLEAR_TARGET, 0xff0000ff, 1.0f);
    Viewport(0, 0, 32, 16);
    Draw(0.9f, 0xffffffff);
    // Clearing both must allow farther geometry only inside its rectangle.
    Viewport(16, 0, 8, 16);
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff0000ff, 1.0f);
    Viewport(0, 0, 32, 16);
    Draw(0.8f, 0xffffff00);

    DWORD pixels[32 * 16];
    Gfx_ReadBackBuffer(0, 0, 32, 16, pixels);
    Gfx_Present(FALSE);
    bool ready = false;
    const Uint64 start = SDL_GetTicks();
    while (!(ready = Gfx_ReadBackBuffer(0, 0, 32, 16, pixels)) && SDL_GetTicks() - start < 5000)
        SDL_Delay(1);
    if (!ready) fprintf(stderr, "gpu clear: timed out waiting for pixels\n");
    bool correct = ready;
    for (int y = 0; ready && y < 16; y++)
        for (int x = 0; x < 32; x++) {
            DWORD expected = x < 8 ? 0xff0000ff : x >= 16 && x < 24 ? 0xffffff00 :
                             x >= 8 && x < 24 && y >= 4 && y < 12 ? 0xff00ff00 : 0xffff0000;
            if (pixels[y * 32 + x] != expected) {
                fprintf(stderr, "gpu clear: pixel %d,%d: %08x, expected %08x\n", x, y,
                        unsigned(pixels[y * 32 + x]), unsigned(expected));
                correct = false;
            }
        }
    Gfx_Shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (!correct) return 1;
    puts("gpu clear: colour, depth and combined viewport clears preserve surrounding pixels");
    return 0;
}
