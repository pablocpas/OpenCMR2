// Localised font atlases define their own byte-to-glyph mapping. Exercise
// the actual replacement path without original assets or a GPU.
#include "port/outline_font.h"
#include "render/gfx_internal.h"
#include "fonts_embedded.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

static void Check(bool ok, const char *message)
{
    if (!ok) {
        fprintf(stderr, "outline font: %s\n", message);
        exit(1);
    }
}

constexpr int kCellWidth = 24, kCellHeight = 32;

static OutlineFontGlyph Cell(BYTE code)
{
    return {code, short((code % 16) * kCellWidth), short((code / 16) * kCellHeight), kCellWidth, kCellHeight};
}

static void Draw(GfxTexture *texture, const stbtt_fontinfo &font, BYTE code, int unicode)
{
    const auto cell = Cell(code);
    const float scale = stbtt_ScaleForMappingEmToPixels(&font, 26);
    int w, h, x0, y0;
    unsigned char *bitmap = stbtt_GetCodepointBitmap(&font, scale, scale, unicode, &w, &h, &x0, &y0);
    Check(bitmap != nullptr, "rasterise fixture glyph");
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const int cx = (kCellWidth - w) / 2 + x, cy = 25 + y0 + y;
            Check(cx >= 0 && cx < kCellWidth && cy >= 0 && cy < kCellHeight, "fixture fits its cell");
            auto *pixel = &texture->levels[0][0][((size_t)(cell.y + cy) * texture->width + cell.x + cx) * 4];
            pixel[0] = pixel[1] = pixel[2] = 255;
            pixel[3] = bitmap[y * w + x];
        }
    stbtt_FreeBitmap(bitmap, nullptr);
}

static bool SameCell(GfxTexture *a, GfxTexture *b, BYTE code)
{
    a = Gfx_DrawnTexture(a);
    b = Gfx_DrawnTexture(b);
    Check(a->width == b->width && a->height == b->height, "replacement keeps atlas dimensions");
    const int scale = a->width / (16 * kCellWidth);
    const auto cell = Cell(code);
    for (int y = cell.y * scale; y < (cell.y + cell.height) * scale; y++) {
        const size_t offset = ((size_t)y * a->width + cell.x * scale) * 4;
        if (memcmp(&a->levels[0][0][offset], &b->levels[0][0][offset], cell.width * scale * 4) != 0)
            return false;
    }
    return true;
}

int main()
{
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "OPENCMR2_HEADLESS", "1", true);
    Check(Gfx_SetVideoMode(640, 480, FALSE), "initialise headless renderer");
    stbtt_fontinfo font;
    Check(stbtt_InitFont(&font, g_texgyreheros_regular_otf, 0), "load fixture font");
    auto *texture = Gfx_CreateTexture(16 * kCellWidth, 16 * kCellHeight, GFX_FORMAT_BGRA8, 1, 0);
    Check(texture != nullptr, "create fixture atlas");
    std::vector<OutlineFontGlyph> cells;
    const char refs[] = "HOaenos08R";
    for (const char *p = refs; *p; p++) {
        Draw(texture, font, *p, *p);
        cells.push_back(Cell(*p));
    }
    // The same Latin-1/CP1252 slots can contain Cyrillic in CP1251. Draw
    // Cyrillic Zhe explicitly so no test depends on an installed font.
    for (int code = 0x80; code <= 0xff; code++) {
        const auto cell = Cell(BYTE(code));
        for (int y = 6; y < 25; y++)
            for (int x = 2; x < 22; x++) {
                const bool ink = (x >= 11 && x <= 13) || abs(x - 12 - (y - 15)) <= 1 ||
                                 abs(x - 12 + (y - 15)) <= 1;
                auto *pixel = &texture->levels[0][0][((size_t)(cell.y + y) * texture->width + cell.x + x) * 4];
                pixel[0] = pixel[1] = pixel[2] = 255;
                pixel[3] = ink ? 255 : 0;
            }
        cells.push_back(cell);
    }
    // Polish repurposes these ASCII symbols; other translations may also
    // remap ASCII letters. Neither may silently become a different glyph.
    const char remapped[] = "$*{|}~Z";
    for (const char *p = remapped; *p; p++) {
        Draw(texture, font, *p, 'H');
        cells.push_back(Cell(*p));
    }
    const auto original = texture->levels[0][0];
    auto *bitmap = Gfx_CreateTexture(texture->width, texture->height, GFX_FORMAT_BGRA8, 1, 0);
    bitmap->levels[0][0] = original;
    OutlineFont_Replace("lcd_fixture.dds", bitmap, cells.data(), int(cells.size()));
    OutlineFont_Replace("hel_fixture.dds", texture, cells.data(), int(cells.size()));
    Check(Gfx_DrawnTexture(texture) != texture, "Helvetica still uses distance fields");
    Check(Gfx_DrawnTexture(texture)->flags & GFX_TEXTURE_DISTANCE_FIELD, "replacement is a distance field");
    Check(texture->levels[0][0] == original, "source atlas stays unchanged");
    for (int code = 0x80; code <= 0xff; code++)
        Check(SameCell(texture, bitmap, BYTE(code)), "extended glyph keeps its original shape");
    for (const char *p = remapped; *p; p++)
        Check(SameCell(texture, bitmap, *p), "remapped ASCII keeps its original shape");
    bool outlined = false;
    for (const char *p = refs; *p; p++)
        outlined |= !SameCell(texture, bitmap, *p);
    Check(outlined, "matching Latin glyphs still use outlines");
    auto *reload = Gfx_CreateTexture(texture->width, texture->height, GFX_FORMAT_BGRA8, 1, 0);
    reload->levels[0][0] = original;
    OutlineFont_Replace("hel_fixture.dds", reload, cells.data(), int(cells.size()));
    Check(Gfx_DrawnTexture(reload)->levels[0][0] == Gfx_DrawnTexture(texture)->levels[0][0],
          "reload preserves cached localised fields");
    Gfx_DestroyTexture(reload);
    Gfx_DestroyTexture(bitmap);
    Gfx_DestroyTexture(texture);
    Gfx_Shutdown();
    puts("outline font: Cyrillic byte slots, remapped ASCII, Latin outlines and reloads passed");
}
