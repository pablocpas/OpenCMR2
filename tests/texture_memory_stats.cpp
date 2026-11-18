// Behavioural checks for Gfx_GetTextureMemoryStats: the overlay's texture
// memory row must reflect real registrations, level bytes and destruction.
// The "opencmr2 eats 10 GB" report needs this counter to separate texture
// retention from other growth. Headless: a Capture executor stands in for
// SDL_GPU, as in france1_textures.cpp.
#include "port/types.h"
#include "render/gfx_internal.h"
#include <cstdio>

extern "C" void Sys_Log(const char *, ...) {}
extern "C" double Sys_GetTicksPrecise(void) { return 0; }
extern "C" void Sys_GetDesktopSize(int *w, int *h) { *w = 640; *h = 480; }
int Platform_GetSettingInt(const char *, int x) { return x; }
const char *Platform_GetSetting(const char *, const char *x) { return x; }
GfxExecutor *Gfx_CreateGpuExecutor()
{
    struct Capture : GfxExecutor {
        bool SetVideoMode(int, int, bool) override { return true; }
        void SetFullscreen(bool) override {}
        void Execute(GfxFrame &, std::vector<GfxTexture *> &, GfxReadback &, bool, bool, bool) override {}
        void DestroyTexture(GfxTexture *) override {}
        void ReadTexture(GfxTexture *) override {}
    };
    return new Capture;
}

#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            fprintf(stderr, "line %d: %s\n", __LINE__, #x);                                                            \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

int main()
{
    CHECK(Gfx_SetVideoMode(640, 480, FALSE));
    unsigned long long cpu = 0, gpu = 0;
    int count = -1;
    Gfx_GetTextureMemoryStats(&cpu, &gpu, &count);
    CHECK(count == 0 && cpu == 0 && gpu == 0);

    // 64x64 BGRA8, explicit full mip chain (7 levels): CPU = sum w*h*4.
    GfxTexture *a = Gfx_CreateTexture(64, 64, GFX_FORMAT_BGRA8, 7, 0);
    Gfx_GetTextureMemoryStats(&cpu, &gpu, &count);
    unsigned long long chain = 0;
    for (int w = 64; w >= 1; w >>= 1)
        chain += (unsigned long long)w * w * 4;
    CHECK(count == 1);
    CHECK(cpu == chain);
    CHECK(gpu == chain); // BGRA8 uploads as-is: 4 bytes per pixel per level

    // 64x64 BC1, one level: CPU keeps the 0.5 bytes/pixel source, the GPU
    // estimate is the decoded 4 bytes/pixel (the Metal upload path decodes).
    GfxTexture *b = Gfx_CreateTexture(64, 64, GFX_FORMAT_BC1, 1, 0);
    Gfx_GetTextureMemoryStats(&cpu, &gpu, &count);
    CHECK(count == 2);
    CHECK(cpu == chain + 64 * 64 / 2);
    CHECK(gpu == chain + 64 * 64 * 4);

    Gfx_DestroyTexture(a);
    Gfx_GetTextureMemoryStats(&cpu, &gpu, &count);
    CHECK(count == 1);
    CHECK(cpu == 64 * 64 / 2);
    Gfx_DestroyTexture(b);
    Gfx_GetTextureMemoryStats(&cpu, &gpu, &count);
    CHECK(count == 0 && cpu == 0 && gpu == 0);
    // Devin #384: target color is not its only allocation. Both backends own one
    // 32-bit depth attachment per target (also for a six-face cube target).
    for (DWORD flags : {DWORD(GFX_TEXTURE_RENDER_TARGET), DWORD(GFX_TEXTURE_RENDER_TARGET | GFX_TEXTURE_CUBE)}) {
        GfxTexture *target = Gfx_CreateTexture(32, 32, GFX_FORMAT_BGRA8, 1, flags);
        Gfx_GetTextureMemoryStats(&cpu, &gpu, &count);
        unsigned long long color = 32 * 32 * 4 * ((flags & GFX_TEXTURE_CUBE) ? 6 : 1);
        CHECK(count == 1 && cpu == color);
        CHECK(gpu == color + 32 * 32 * 4);
        Gfx_DestroyTexture(target);
        Gfx_GetTextureMemoryStats(&cpu, &gpu, &count);
        CHECK(count == 0 && cpu == 0 && gpu == 0);
    }
    Gfx_Shutdown();
    puts("texture memory stats: registration, level bytes, BC decode estimate, destruction");
    return 0;
}
