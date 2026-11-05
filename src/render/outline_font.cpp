// Distance fields for the game's bitmap fonts (port/outline_font.h).
//
// The game draws text glyph by glyph as sprites of its font textures, cells
// in pixels for 640x480 or 1024x768: scaled up, their edges blur or step.
// Each texture gets a replacement with the same cells, N texels per pixel,
// whose alpha is the signed distance to the glyph's edge; the shader draws
// that edge one pixel wide at any scale. The cells, widths and kerning stay
// the original's, so text is laid out as before.
//
// The Helvetica fonts (hel_*) are drawn from TeX Gyre Heros, a Helvetica
// with a free licence: its size, weight and baseline are fitted to a few of
// the font's glyphs, and each glyph is centred where the original's ink is.
// Other fonts and non-ASCII glyphs are reconstructed from their bitmaps.
// Localised editions can assign different letters to the same byte codes;
// their font atlases, rather than a Western code page, define those letters.
// Distances are measured on a coverage of the glyph oversampled again, so
// the cubic outlines of the OpenType fonts are followed exactly.

#include "port/outline_font.h"
#include "platform/platform.h"
#include "port/sys.h"
#include "fonts_embedded.h"
#include "distance_field.h"

#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

namespace {

// Distance, in pixels of the font, the field holds on either side of an edge.
const float kSpread = 2.0f;
// Coverage samples per texel of the field, along each axis.
const int kOversample = 4;

// The font texture, 0..1 per channel.
struct Bitmap {
    int width = 0, height = 0;
    std::vector<float> r, g, b, a;
    float At(const std::vector<float> &c, int x, int y) const
    {
        return x < 0 || y < 0 || x >= width || y >= height ? 0.0f : c[(size_t)y * width + x];
    }
};

struct Cell {
    int x, y, w, h;
};

struct Face {
    stbtt_fontinfo info;
    const char *name;
    bool ok = false;
};

Face *Faces()
{
    static Face faces[2];
    static bool loaded;
    if (!loaded) {
        loaded = true;
        faces[0].name = "regular";
        faces[0].ok = stbtt_InitFont(&faces[0].info, g_texgyreheros_regular_otf, 0) != 0;
        faces[1].name = "bold";
        faces[1].ok = stbtt_InitFont(&faces[1].info, g_texgyreheros_bold_otf, 0) != 0;
    }
    return faces;
}

// A glyph's coverage at a scale (pixels per em) and subpixel shift: the
// pixels and where the first one is from the pen (y down).
struct Raster {
    int x0 = 0, y0 = 0, w = 0, h = 0;
    std::vector<unsigned char> pixels;
};

Raster Rasterize(const Face &face, int glyph, float scaleX, float scaleY, float shiftX, float shiftY)
{
    Raster r;
    int x1, y1;
    stbtt_GetGlyphBitmapBoxSubpixel(&face.info, glyph, scaleX, scaleY, shiftX, shiftY, &r.x0, &r.y0, &x1, &y1);
    r.w = x1 - r.x0;
    r.h = y1 - r.y0;
    if (r.w <= 0 || r.h <= 0) {
        r.w = r.h = 0;
        return r;
    }
    r.pixels.assign((size_t)r.w * r.h, 0);
    stbtt_MakeGlyphBitmapSubpixel(&face.info, r.pixels.data(), r.w, r.h, r.w, scaleX, scaleY, shiftX, shiftY, glyph);
    return r;
}

// Horizontal centre of a raster's ink, from the pen, in its pixels.
float CentreX(const Raster &r)
{
    double mass = 0, sum = 0;
    for (int y = 0; y < r.h; y++)
        for (int x = 0; x < r.w; x++) {
            double v = r.pixels[(size_t)y * r.w + x];
            mass += v;
            sum += v * (r.x0 + x + 0.5);
        }
    return mass > 0 ? (float)(sum / mass) : 0.0f;
}

// The sum of each column's highest coverage, in its pixels.
float CoverageWidth(const Raster &r)
{
    double width = 0;
    for (int x = 0; x < r.w; x++) {
        int column = 0;
        for (int y = 0; y < r.h; y++)
            column = std::max<int>(column, r.pixels[(size_t)y * r.w + x]);
        width += column / 255.0;
    }
    return (float)width;
}

// Ink of a cell of the bitmap: its mass and horizontal centre (from the
// cell's left), and its rows at half coverage or more.
struct Ink {
    float mass = 0, centreX = 0;
    float width = 0;    // the sum of each column's highest coverage
    int top = -1, bottom = -1;
};

Ink Measure(const Bitmap &bm, const Cell &c)
{
    Ink ink;
    double sum = 0;
    for (int y = 0; y < c.h; y++)
        for (int x = 0; x < c.w; x++) {
            float v = bm.At(bm.a, c.x + x, c.y + y);
            ink.mass += v;
            sum += v * (x + 0.5);
            if (v >= 0.5f) {
                if (ink.top < 0)
                    ink.top = y;
                ink.bottom = y + 1;
            }
        }
    for (int x = 0; x < c.w; x++) {
        float column = 0;
        for (int y = 0; y < c.h; y++)
            column = std::max(column, bm.At(bm.a, c.x + x, c.y + y));
        ink.width += column;
    }
    if (ink.mass > 0)
        ink.centreX = (float)(sum / ink.mass);
    return ink;
}

// Where Heros puts a glyph in a cell: its pen, from the cell's corner, in
// pixels of the font.
struct Fit {
    const Face *face = nullptr;
    float size = 0;         // pixels per em
    float baseline = 0;     // from the top of the cells
    float weight = 0;       // pixels the edges move out (bolder) or in
    float error = 0;        // per unit of ink
};

// Samples per pixel of the coverage a fit is measured on.
const int kFitOversample = 4;

// Draws a glyph in a cell at a scale of the font's pixels, centred on the
// original's ink and, with a width, as wide as it (within 10%: Heros's
// glyphs are up to a pixel wider than the bitmaps', which closes the gaps
// between letters); adds its coverage to out (cell pixels times scale).
// A glyph wider than its cell is narrowed to it, and kept inside it; what
// would leave it at the top or bottom, Heros's longer descenders and the
// accents over its capitals, is pressed into it, half a pixel inside, the
// rest of the glyph keeping its size.
void PlaceGlyph(const Fit &fit, int glyph, const Ink &ink, bool matchWidth, int scaleUp, int cellW, int cellH,
                std::vector<float> &out)
{
    const float em = stbtt_ScaleForMappingEmToPixels(&fit.face->info, fit.size) * scaleUp;
    float emX = em;
    Raster centred = Rasterize(*fit.face, glyph, emX, em, 0.0f, 0.0f);
    float width = CoverageWidth(centred) / scaleUp;
    if (matchWidth && width >= 3.0f && ink.width >= 3.0f) {
        emX = em * std::min(std::max(ink.width / width, 0.9f), 1.1f);
        centred = Rasterize(*fit.face, glyph, emX, em, 0.0f, 0.0f);
    }
    const int w = cellW * scaleUp, h = cellH * scaleUp;
    // No wider than the cell: the original's glyphs fill theirs.
    if (centred.w > w) {
        emX *= (float)w / centred.w;
        centred = Rasterize(*fit.face, glyph, emX, em, 0.0f, 0.0f);
    }
    float penX = ink.centreX * scaleUp - CentreX(centred);
    // Moved inside the cell where centring would take it out.
    penX = std::max(penX, (float)-centred.x0);
    penX = std::min(penX, (float)(w - centred.x0 - centred.w));
    float penY = fit.baseline * scaleUp;
    int ox = (int)floorf(penX), oy = (int)floorf(penY);
    Raster r = Rasterize(*fit.face, glyph, emX, em, penX - ox, penY - oy);
    if (r.h == 0)
        return;
    // Rows of the raster from the cell's top, and the bands pressed in.
    const float top = (float)(oy + r.y0), bottom = top + r.h;
    const float lowLimit = (cellH - 0.5f) * scaleUp, highLimit = 0.5f * scaleUp;
    int x0, y0, x1, y1;
    float capTop = penY;
    if (stbtt_GetCodepointBox(&fit.face->info, 'H', &x0, &y0, &x1, &y1))
        capTop = penY - y1 * em;
    const float low = bottom > lowLimit && lowLimit > penY ? (lowLimit - penY) / (bottom - penY) : 1.0f;
    const float high = top < highLimit && capTop > highLimit ? (capTop - highLimit) / (capTop - top) : 1.0f;
    for (int cy = 0; cy < h; cy++) {
        // The raster's row this row of the cell shows (at its centre).
        float y = cy + 0.5f;
        if (low < 1.0f && y > penY)
            y = penY + (y - penY) / low;
        else if (high < 1.0f && y < capTop)
            y = capTop - (capTop - y) / high;
        float ry = y - top - 0.5f;
        int iy = (int)floorf(ry);
        float t = ry - iy;
        if (iy < -1 || iy >= r.h)
            continue;
        for (int x = 0; x < r.w; x++) {
            int cx = ox + r.x0 + x;
            if (cx < 0 || cx >= w)
                continue;
            float a = iy >= 0 ? r.pixels[(size_t)iy * r.w + x] : 0;
            float b = iy + 1 < r.h ? r.pixels[(size_t)(iy + 1) * r.w + x] : 0;
            out[(size_t)cy * w + cx] += (a + (b - a) * t) / 255.0f;
        }
    }
}

// How far a fit's glyphs are from the original's, in coverage per unit of
// ink, at the best of its weights from minWeight to maxWeight (which it sets).
float FitError(Fit &fit, const Bitmap &bm, const std::vector<std::pair<Cell, int>> &refs, float minWeight,
               float maxWeight)
{
    const int up = kFitOversample;
    const int steps = 9;
    double error[steps] = {}, mass = 0;
    std::vector<float> drawn;
    for (const auto &ref : refs) {
        const Cell &c = ref.first;
        Ink ink = Measure(bm, c);
        const int w = c.w * up, h = c.h * up;
        drawn.assign((size_t)w * h, 0.0f);
        PlaceGlyph(fit, ref.second, ink, false, up, c.w, c.h, drawn);
        std::vector<float> distance = DistanceField::FromCoverage(drawn, w, h);
        for (int s = 0; s < steps; s++) {
            const float weight = (minWeight + (maxWeight - minWeight) * s / (steps - 1)) * up;
            for (int y = 0; y < c.h; y++)
                for (int x = 0; x < c.w; x++) {
                    int inside = 0;
                    for (int sy = 0; sy < up; sy++)
                        for (int sx = 0; sx < up; sx++)
                            inside += distance[(size_t)(y * up + sy) * w + x * up + sx] + weight > 0;
                    error[s] += fabs((float)inside / (up * up) - bm.At(bm.a, c.x + x, c.y + y));
                }
        }
        mass += ink.mass;
    }
    int best = 0;
    for (int s = 1; s < steps; s++)
        if (error[s] < error[best])
            best = s;
    fit.weight = minWeight + (maxWeight - minWeight) * best / (steps - 1);
    return mass > 0 ? (float)(error[best] / mass) : 1e9f;
}

// Fits Heros to a Helvetica font: face, size and weight on a coarse grid,
// then the baseline, size and weight finer. Returns a fit without face if no glyph
// to measure is there.
Fit FitHeros(const Bitmap &bm, const std::vector<OutlineFontGlyph> &glyphs)
{
    Fit best;
    Face *faces = Faces();
    if (!faces[0].ok || !faces[1].ok)
        return best;
    static const char refChars[] = "HOaenos08R";
    std::vector<std::pair<Cell, int>> refs[2];
    const OutlineFontGlyph *flat = nullptr;     // a glyph standing on the baseline
    for (const char *p = refChars; *p; p++)
        for (const OutlineFontGlyph &g : glyphs)
            if (g.code == (BYTE)*p) {
                Cell c = { g.x, g.y, g.width, g.height };
                if (Measure(bm, c).mass <= 0)
                    break;
                for (int f = 0; f < 2; f++)
                    refs[f].push_back({ c, stbtt_FindGlyphIndex(&faces[f].info, g.code) });
                if (flat == nullptr && strchr("H08", *p) != nullptr)
                    flat = &g;
                break;
            }
    if (refs[0].size() < 3 || flat == nullptr)
        return best;
    Ink flatInk = Measure(bm, { flat->x, flat->y, flat->width, flat->height });
    const float height = (float)(flatInk.bottom - flatInk.top);
    best.error = 1e9f;
    for (int f = 0; f < 2; f++)
        for (float size = height * 1.2f; size <= height * 1.6f; size += 0.2f) {
            Fit fit;
            fit.face = &faces[f];
            fit.size = size;
            fit.baseline = (float)flatInk.bottom;
            fit.error = FitError(fit, bm, refs[f], -0.3f, 0.3f);
            if (fit.error < best.error)
                best = fit;
        }
    const int f = best.face == &faces[1];
    Fit coarse = best;
    for (float dy = -0.5f; dy <= 0.5f; dy += 0.125f)
        for (float ds = -0.2f; ds <= 0.2f; ds += 0.05f) {
            Fit fit = coarse;
            fit.size = coarse.size + ds;
            fit.baseline = coarse.baseline + dy;
            fit.error = FitError(fit, bm, refs[f], coarse.weight - 0.15f, coarse.weight + 0.15f);
            if (fit.error < best.error)
                best = fit;
        }
    return best;
}

// Catmull-Rom weights of the four samples around t in [0, 1).
void CubicWeights(float t, float w[4])
{
    float t2 = t * t, t3 = t2 * t;
    w[0] = 0.5f * (-t3 + 2 * t2 - t);
    w[1] = 0.5f * (3 * t3 - 5 * t2 + 2);
    w[2] = 0.5f * (-3 * t3 + 4 * t2 + t);
    w[3] = 0.5f * (t3 - t2);
}

// A cell's bitmap coverage at a scale, smoothly interpolated.
void Upsample(const Bitmap &bm, const Cell &c, int scaleUp, std::vector<float> &out)
{
    const int w = c.w * scaleUp, h = c.h * scaleUp;
    auto at = [&](int x, int y) {
        return x < 0 || y < 0 || x >= c.w || y >= c.h ? 0.0f : bm.At(bm.a, c.x + x, c.y + y);
    };
    for (int y = 0; y < h; y++) {
        float sy = (y + 0.5f) / scaleUp - 0.5f;
        int iy = (int)floorf(sy);
        float wy[4];
        CubicWeights(sy - iy, wy);
        for (int x = 0; x < w; x++) {
            float sx = (x + 0.5f) / scaleUp - 0.5f;
            int ix = (int)floorf(sx);
            float wx[4];
            CubicWeights(sx - ix, wx);
            float v = 0;
            for (int j = 0; j < 4; j++)
                for (int i = 0; i < 4; i++)
                    v += wy[j] * wx[i] * at(ix - 1 + i, iy - 1 + j);
            out[(size_t)y * w + x] = v;
        }
    }
}

// The distance field of a cell from its coverage at texels * kOversample
// per pixel, its edges moved out by weight pixels: alpha 0.5 on the edge, 0
// and 1 kSpread pixels out and in.
void WriteField(const std::vector<float> &coverage, const Cell &c, int texels, float weight, float rgb[3],
                std::vector<uint8_t> &out, int outWidth, int outHeight)
{
    const int up = texels * kOversample;
    const int w = c.w * up, h = c.h * up;
    std::vector<float> distance = DistanceField::FromCoverage(coverage, w, h);
    for (int ty = 0; ty < c.h * texels; ty++) {
        int oy = c.y * texels + ty;
        if (oy < 0 || oy >= outHeight)
            continue;
        for (int tx = 0; tx < c.w * texels; tx++) {
            int ox = c.x * texels + tx;
            if (ox < 0 || ox >= outWidth)
                continue;
            // The mean signed distance over the texel's samples.
            float sum = 0;
            for (int sy = 0; sy < kOversample; sy++)
                for (int sx = 0; sx < kOversample; sx++)
                    sum += distance[(size_t)(ty * kOversample + sy) * w + tx * kOversample + sx];
            float a = 0.5f + (sum / (kOversample * kOversample) / up + weight) / (2.0f * kSpread);
            uint8_t *p = &out[((size_t)oy * outWidth + ox) * 4];
            p[0] = (uint8_t)lroundf(rgb[2] * 255.0f);
            p[1] = (uint8_t)lroundf(rgb[1] * 255.0f);
            p[2] = (uint8_t)lroundf(rgb[0] * 255.0f);
            p[3] = (uint8_t)lroundf(std::min(std::max(a, 0.0f), 1.0f) * 255.0f);
        }
    }
}

struct Built {
    int width = 0, height = 0;
    std::vector<uint8_t> pixels;    // BGRA, the top level
};

bool ReadBitmap(GfxTexture *texture, Bitmap &bm)
{
    GfxLockedRect r;
    if (!Gfx_LockTexture(texture, 0, 0, &r))
        return false;
    Gfx_UnlockTexture(texture, 0, 0);
    GfxTexture *copy = Gfx_CreateTexture(r.width, r.height, GFX_FORMAT_BGRA8, 1, GFX_TEXTURE_STAGING);
    if (copy == NULL)
        return false;
    Gfx_CopyTexture(copy, texture);
    bool ok = Gfx_LockTexture(copy, 0, 0, &r) != 0;
    if (ok) {
        bm.width = r.width;
        bm.height = r.height;
        size_t n = (size_t)bm.width * bm.height;
        bm.r.resize(n);
        bm.g.resize(n);
        bm.b.resize(n);
        bm.a.resize(n);
        for (int y = 0; y < bm.height; y++) {
            const uint8_t *p = (const uint8_t *)r.pixels + (size_t)y * r.pitch;
            for (int x = 0; x < bm.width; x++, p += 4) {
                size_t i = (size_t)y * bm.width + x;
                bm.b[i] = p[0] / 255.0f;
                bm.g[i] = p[1] / 255.0f;
                bm.r[i] = p[2] / 255.0f;
                bm.a[i] = p[3] / 255.0f;
            }
        }
        Gfx_UnlockTexture(copy, 0, 0);
    }
    Gfx_DestroyTexture(copy);
    return ok;
}

Built Build(const std::string &name, const Bitmap &bm, const std::vector<OutlineFontGlyph> &glyphs)
{
    Built built;
    std::string base = name.substr(name.find_last_of("\\/") + 1);
    std::transform(base.begin(), base.end(), base.begin(), [](char c) { return (char)tolower((unsigned char)c); });
    Fit fit;
    if (base.compare(0, 4, "hel_") == 0)
        fit = FitHeros(bm, glyphs);
    // Glyphs of about 50 texels high: enough for the corners at 4K.
    int lineHeight = 1;
    for (const OutlineFontGlyph &g : glyphs)
        lineHeight = std::max<int>(lineHeight, g.height);
    const int texels = std::min(4, std::max(2, (int)ceilf(72.0f / lineHeight)));
    built.width = bm.width * texels;
    built.height = bm.height * texels;
    built.pixels.assign((size_t)built.width * built.height * 4, 0);
    // Outside the cells: white, far from any edge.
    for (size_t i = 0; i < built.pixels.size(); i += 4)
        built.pixels[i] = built.pixels[i + 1] = built.pixels[i + 2] = 255;
    const int up = texels * kOversample;
    // A cell's coverage at the field's samples: Heros's glyph, or the
    // bitmap's for localised glyphs and other fonts. Returns whether the
    // glyph is Heros's.
    std::vector<float> coverage;
    auto draw = [&](const OutlineFontGlyph &g, const Cell &c, const Ink &ink) {
        coverage.assign((size_t)c.w * up * c.h * up, 0.0f);
        if (ink.mass <= 0)
            return false;
        // Only ASCII letters and digits are candidates for outlines.
        // In particular, CP1251 Cyrillic occupies the same byte codes as
        // CP1252 accented Latin. Polish also repurposes ASCII punctuation.
        const bool latin = (g.code >= 'A' && g.code <= 'Z') || (g.code >= 'a' && g.code <= 'z') ||
                           (g.code >= '0' && g.code <= '9');
        int glyph = fit.face != nullptr && latin ? stbtt_FindGlyphIndex(&fit.face->info, g.code) : 0;
        if (glyph == 0) {
            Upsample(bm, c, up, coverage);
            return false;
        }
        PlaceGlyph(fit, glyph, ink, true, up, c.w, c.h, coverage);
        // Translation fonts can even replace ASCII letters. Reject an
        // outline whose mean coverage error exceeds 35% of the source ink.
        double error = 0;
        for (int y = 0; y < c.h; y++)
            for (int x = 0; x < c.w; x++) {
                float alpha = 0;
                for (int sy = 0; sy < up; sy++)
                    for (int sx = 0; sx < up; sx++)
                        alpha += coverage[(size_t)(y * up + sy) * c.w * up + x * up + sx];
                error += fabs(alpha / (up * up) - bm.At(bm.a, c.x + x, c.y + y));
            }
        if (error > ink.mass * 0.35f) {
            Upsample(bm, c, up, coverage);
            return false;
        }
        return true;
    };
    int outlined = 0;
    for (const OutlineFontGlyph &g : glyphs) {
        Cell c = { g.x, g.y, g.width, g.height };
        if (c.w <= 0 || c.h <= 0)
            continue;
        Ink ink = Measure(bm, c);
        // The colour of the glyph's ink.
        float rgb[3] = { 1, 1, 1 };
        if (ink.mass > 0) {
            double sum[3] = { 0, 0, 0 };
            for (int y = 0; y < c.h; y++)
                for (int x = 0; x < c.w; x++) {
                    float a = bm.At(bm.a, c.x + x, c.y + y);
                    sum[0] += a * bm.At(bm.r, c.x + x, c.y + y);
                    sum[1] += a * bm.At(bm.g, c.x + x, c.y + y);
                    sum[2] += a * bm.At(bm.b, c.x + x, c.y + y);
                }
            for (int i = 0; i < 3; i++)
                rgb[i] = (float)(sum[i] / ink.mass);
        }
        bool heros = draw(g, c, ink);
        outlined += heros;
        WriteField(coverage, c, texels, heros ? fit.weight : 0.0f, rgb, built.pixels, built.width, built.height);
    }
    if (fit.face != nullptr)
        Sys_Log("fonts: %s: TeX Gyre Heros %s at %.2f px, weight %+.2f px, baseline %.2f, error %.3f; "
                "%d of %d glyphs outlined",
                base.c_str(), fit.face->name, fit.size, fit.weight, fit.baseline, fit.error, outlined,
                (int)glyphs.size());
    else
        Sys_Log("fonts: %s: %d glyphs from the bitmap", base.c_str(), (int)glyphs.size());
    return built;
}

} // namespace

extern "C" void OutlineFont_Replace(const char *name, GfxTexture *texture, const OutlineFontGlyph *glyphs, int count)
{
    if (texture == NULL || name == NULL || count <= 0 || !Platform_GetSettingInt("video.outline_fonts", 1))
        return;
    // Fonts are loaded again with every menu and race: each is built once.
    static std::map<std::string, Built> cache;
    std::vector<OutlineFontGlyph> list(glyphs, glyphs + count);
    Bitmap bm;
    GfxLockedRect r;
    if (!ReadBitmap(texture, bm))
        return;
    // The stage fonts of the 640x480 and 1024x768 archives share names and
    // sizes (hel_13pt, lcd_640) but not their glyphs: the key is what the
    // field is built from, the texels and the cells.
    uint64_t hash = 14695981039346656037ull;
    auto mix = [&hash](const void *p, size_t n) {
        for (size_t i = 0; i < n; i++)
            hash = (hash ^ ((const uint8_t *)p)[i]) * 1099511628211ull;
    };
    for (const std::vector<float> *c : { &bm.r, &bm.g, &bm.b, &bm.a })
        mix(c->data(), c->size() * sizeof(float));
    for (const OutlineFontGlyph &g : list) {
        const short cell[5] = { g.code, g.x, g.y, g.width, g.height };
        mix(cell, sizeof(cell));
    }
    std::string key = std::string(name) + "|" + std::to_string(bm.width) + "x" + std::to_string(bm.height) + "|" +
                      std::to_string(hash);
    auto it = cache.find(key);
    if (it == cache.end())
        it = cache.emplace(key, Build(name, bm, list)).first;
    const Built &built = it->second;
    int levels = 1;
    while ((std::max(built.width, built.height) >> levels) > 0 && levels < 16)
        levels++;
    GfxTexture *field = Gfx_CreateTexture(built.width, built.height, GFX_FORMAT_BGRA8, levels,
                                          GFX_TEXTURE_DISTANCE_FIELD);
    if (field == NULL || !Gfx_LockTexture(field, 0, 0, &r)) {
        Gfx_DestroyTexture(field);
        return;
    }
    for (int y = 0; y < built.height; y++)
        memcpy((uint8_t *)r.pixels + (size_t)y * r.pitch, &built.pixels[(size_t)y * built.width * 4], built.width * 4);
    Gfx_UnlockTexture(field, 0, 0);
    Gfx_GenerateMipmaps(field);
    Gfx_SetTextureReplacement(texture, field);
}
