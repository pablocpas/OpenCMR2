// The original 18x12 flag sprites, compared with captures of v1.6.2.
// No game data or GPU is required. The bitmap stays intact at native size.
// menu-flags.txt was captured from the unmodified Frontend.cpp object
// (git blob 710ae62e991af90375d01b239b4cb8f22a2faf4a): 16/32-pixel
// texture sets, times 1000/1600 ms, then matrix maps 0..10 in each case.
#include "port/types.h"
#include "render/gfx_internal.h"
#include "Frontend.h"
#include "Graphics.h"
#include "Game.h"
#include "Sprite.h"
#include "main.h"

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

extern void Sprite_DrawLayer(int);
extern void Scene_InitFixedMathTables();
extern void FrontendDraw_AnimatedMatrixBackground(short, short, char *);
extern void FrontendDraw_MatrixMedalRect(int, int, int);
extern char g_matrixMaps[11][216];

static void Check(bool ok, const char *message)
{
    if (!ok) {
        fprintf(stderr, "country flags: %s\n", message);
        exit(1);
    }
}

// Hash explicit little-endian fields, without native pointers or padding.
static uint64_t HashSprites()
{
    uint64_t hash = 14695981039346656037ull;
    auto word = [&](int value) {
        for (int i = 0; i < 4; i++)
            hash = (hash ^ ((unsigned(value) >> (i * 8)) & 255)) * 1099511628211ull;
    };
    for (unsigned i = 0; i < g_spriteCount1; i++) {
        const Sprite &sprite = g_spriteLayer1[i];
        for (SpriteRect rect : {sprite.src, sprite.dst, sprite.uv2}) {
            word(rect.x); word(rect.y); word(rect.w); word(rect.h);
        }
        for (BYTE channel : sprite.colour) word(channel);
        word(sprite.layer); word(sprite.angle);
        word(sprite.subpixel[0]); word(sprite.subpixel[1]);
        for (int coordinate : sprite.centre) word(coordinate);
        word(sprite.param);
    }
    return hash;
}

static void FillDot(Texture &dot, int size)
{
    dot.width = dot.height = short(size);
    dot.pSurface = Gfx_CreateTexture(size, size, GFX_FORMAT_BGRA8, 1, 0);
    GfxLockedRect lock{};
    Check(Gfx_LockTexture(dot.pSurface, 0, 0, &lock), "lock synthetic dot bitmap");
    for (int y = 0; y < size; y++) {
        auto *row = (unsigned char *)lock.pixels + y * lock.pitch;
        for (int x = 0; x < size; x++) {
            row[x * 4] = row[x * 4 + 1] = row[x * 4 + 2] = 255;
            row[x * 4 + 3] = BYTE((x * 17 + y * 13) & 255);
        }
    }
    Gfx_UnlockTexture(dot.pSurface, 0, 0);
}

static void CheckOriginalPixels(Texture &dot)
{
    GfxTexture *drawn = Gfx_DrawnTexture(dot.pSurface);
    Check(drawn != dot.pSurface && (drawn->flags & GFX_TEXTURE_MENU_DOT), "dot gets a geometric replacement");
    Check(drawn->width == dot.width && drawn->height == dot.height && drawn->format == dot.pSurface->format &&
          drawn->levels[0][0] == dot.pSurface->levels[0][0], "replacement preserves every native bitmap pixel");
    Check(Gfx_GetSpriteTexelInset(dot.pSurface) == 0, "enlarged circles are not cropped to texel centres");
    Gfx_VectorizeMenuDot(dot.pSurface);
    Check(Gfx_DrawnTexture(dot.pSurface) == drawn, "replacement is reused across menu frames");
}

int main(int argc, char **argv)
{
    Check(argc == 2, "expected the original sprite reference file");
    FILE *reference = fopen(argv[1], "r");
    Check(reference != nullptr, "open original sprite reference");
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "OPENCMR2_HEADLESS", "1", true);
    Scene_InitFixedMathTables();
    Graphics graphics{};
    g_pGraphics = &graphics;
    for (int size : {16, 32}) {
        graphics.resX = size == 16 ? 640 : 1024;
        graphics.resY = size == 16 ? 480 : 768;
        Check(Gfx_SetVideoMode(graphics.resX, graphics.resY, FALSE), "create or recreate renderer");
        Texture compressed{};
        compressed.width = compressed.height = short(size);
        compressed.pSurface = Gfx_CreateTexture(size, size, GFX_FORMAT_BC3, 1, 0);
        auto &blocks = compressed.pSurface->levels[0][0];
        for (size_t i = 0; i < blocks.size(); i++)
            blocks[i] = BYTE(i * 17);
        Gfx_VectorizeMenuDot(compressed.pSurface);
        CheckOriginalPixels(compressed);
        Gfx_DestroyTexture(compressed.pSurface);
        Texture dot{};
        FillDot(dot, size);
        CFrontend::m_matrixTextures[FRONTEND_LG_MATRIX] = &dot;
        Check(Gfx_GetSpriteTexelInset(dot.pSurface) == 0.5f, "original bitmap sampling is unchanged");
        CMain::ResetFrameDelta();
        for (int frame : {1000, 1600}) {
            CMain::m_frameTime = frame;
            for (int flag = 0; flag < 11; flag++) {
                FrontendDraw_AnimatedMatrixBackground(160, 100, g_matrixMaps[flag]);
                Check(g_spriteCount1 == 216, "flag retains exactly 18x12 dots");
                for (unsigned i = 0; i < g_spriteCount1; i++)
                    Check(g_spriteLayer1[i].pTexture == &dot, "every cell still uses the original dot sprite");
                unsigned long long expected = 0;
                Check(fscanf(reference, "%llx", &expected) == 1, "read original sprite hash");
                Check(HashSprites() == expected, "dot positions, sizes, colours and ripple match v1.6.2 exactly");
                CheckOriginalPixels(dot);
                Sprite_DrawLayer(1);
                Gfx_Present(FALSE);
            }
        }
        // The medal cells draw the trophy of the place (1 bronze, 2 silver,
        // 3 gold) at its own size (#44: every cell showed a country banner).
        Texture trophies[3]{}, banners[8]{};
        for (int i = 0; i < 3; i++) {
            trophies[i].width = short(40 + i);
            trophies[i].height = short(24 + i);
            CFrontend::m_matrixTextures[FRONTEND_TROPHY_BRONZE + i] = &trophies[i];
        }
        for (int i = 0; i < 8; i++)
            CFrontend::m_pSetupRepBanners[i] = &banners[i];
        for (int place = 0; place <= 3; place++) {
            Sprite_DrawLayer(1);
            FrontendDraw_MatrixMedalRect(320, 200, place);
            const Sprite *trophy = nullptr;
            for (unsigned i = 0; i < g_spriteCount1; i++) {
                Check(g_spriteLayer1[i].pTexture == nullptr || g_spriteLayer1[i].pTexture == &trophies[place - 1],
                      "a medal cell draws only its place's trophy");
                if (g_spriteLayer1[i].pTexture != nullptr)
                    trophy = &g_spriteLayer1[i];
            }
            Check((trophy != nullptr) == (place != 0), "a won place has its trophy, no place has none");
            if (trophy != nullptr)
                Check(trophy->dst.w == trophies[place - 1].width && trophy->dst.h == trophies[place - 1].height &&
                      trophy->dst.x == 320 - trophies[place - 1].width / 2, "the trophy keeps its size, centred");
        }
        for (int i = 0; i < 8; i++)
            CFrontend::m_pSetupRepBanners[i] = nullptr;
        for (int i = 1; i < 4; i++)
            CFrontend::m_matrixTextures[i] = nullptr;
        Sprite_DrawLayer(1);
        Gfx_Shutdown();
    }
    unsigned long long extra = 0;
    Check(fscanf(reference, "%llx", &extra) == EOF, "all original sprite references were checked");
    fclose(reference);
    CFrontend::m_matrixTextures[FRONTEND_LG_MATRIX] = nullptr;
    CGame::UnwindCallbacks(0);
    g_pGraphics = nullptr;
    puts("country flags: all 44 original dot layouts, colours and animation frames preserved");
}
