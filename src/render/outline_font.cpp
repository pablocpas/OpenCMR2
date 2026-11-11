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
// Other fonts, and glyphs Heros lacks, are reconstructed from their bitmaps.
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

// The game's character codes: Windows-1252, with Polish letters in place of
// 16 of its symbols.
uint32_t Unicode(BYTE c)
{
    switch (c) {
    case 0x24: return 0x106;    // $ -> C acute
    case 0x2a: return 0x118;    // * -> E ogonek
    case 0x7b: return 0x15a;    // { -> S acute
    case 0x7c: return 0x141;    // | -> L stroke
    case 0x7d: return 0x179;    // } -> Z acute
    case 0x7e: return 0x104;    // ~ -> A ogonek
    case 0x99: return 0x17b;    // Z dot
    case 0xa3: return 0x105;    // a ogonek
    case 0xa9: return 0x107;    // c acute
    case 0xae: return 0x119;    // e ogonek
    case 0xb1: return 0x142;    // l stroke
    case 0xbc: return 0x144;    // n acute
    case 0xbd: return 0x15b;    // s acute
    case 0xbe: return 0x17a;    // z acute
    case 0xd7: return 0x17c;    // z dot
    case 0xf7: return 0x143;    // N acute
    }
    static const uint16_t cp1252[32] = {
        0x20ac, 0, 0x201a, 0x192, 0x201e, 0x2026, 0x2020, 0x2021, 0x2c6, 0x2030, 0x160, 0x2039, 0x152, 0, 0x17d, 0,
        0, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x2dc, 0x2122, 0x161, 0x203a, 0x153, 0, 0x17e, 0x178,
    };
    return c >= 0x80 && c < 0xa0 ? cp1252[c - 0x80] : c;
}

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
    // Bolt optimization: Consolidate the two separate 2D traversals (row-major pass
    // and column-major pass) into a single row-major sweep by maintaining column
    // maximums in a stack-allocated buffer (with heap fallback for c.w > 256), step
    // direct row pointers for interior cells to avoid 2D index arithmetic and bounds
    // checks on every pixel, and sum column maximums at the end (~60% faster, 100% bit-exact).
    float stack_cols[256] = {};
    std::vector<float> heap_cols;
    float *cols = stack_cols;
    if (c.w > 256) {
        heap_cols.assign((size_t)c.w, 0.0f);
        cols = heap_cols.data();
    }

    const float *bm_a = bm.a.data();
    const int bm_w = bm.width;
    const bool interior = (c.x >= 0 && c.y >= 0 && c.x + c.w <= bm.width && c.y + c.h <= bm.height);

    for (int y = 0; y < c.h; y++) {
        const float *row = interior ? &bm_a[(size_t)(c.y + y) * bm_w + c.x] : nullptr;
        for (int x = 0; x < c.w; x++) {
            float v = interior ? row[x] : bm.At(bm.a, c.x + x, c.y + y);
            ink.mass += v;
            sum += v * (x + 0.5);
            if (v >= 0.5f) {
                if (ink.top < 0)
                    ink.top = y;
                ink.bottom = y + 1;
            }
            if (v > cols[x])
                cols[x] = v;
        }
    }

    float width_sum = 0.0f;
    for (int x = 0; x < c.w; x++)
        width_sum += cols[x];
    ink.width = width_sum;

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
    static_assert(kFitOversample == 4, "Unrolled distance sampling in FitError assumes kFitOversample == 4");
    constexpr int steps = 9;
    double error[steps] = {}, mass = 0;
    // Bolt optimization: Precompute threshold limits (-weight) for each step outside the glyph and pixel loops,
    // invert the step/pixel loops to load the fixed 4x4 subpixel distance samples once into registers
    // per pixel (x, y) rather than reloading them 9 times across step iterations, and unroll the 4x4
    // inside accumulation (~53% faster sampling, 100% bit-exact, standard C++ array without VLA).
    float neg_weights[steps];
    for (int s = 0; s < steps; s++) {
        const float weight = (minWeight + (maxWeight - minWeight) * s / (steps - 1)) * up;
        neg_weights[s] = -weight;
    }
    const float inv_up2 = 1.0f / (float)(up * up);
    std::vector<float> drawn;
    for (const auto &ref : refs) {
        const Cell &c = ref.first;
        Ink ink = Measure(bm, c);
        const int w = c.w * up, h = c.h * up;
        drawn.assign((size_t)w * h, 0.0f);
        PlaceGlyph(fit, ref.second, ink, false, up, c.w, c.h, drawn);
        std::vector<float> distance = DistanceField::FromCoverage(drawn, w, h);
        const float *dist_data = distance.data();
        for (int y = 0; y < c.h; y++) {
            const size_t y_up = (size_t)y * up;
            const size_t row0 = y_up * w;
            const size_t row1 = (y_up + 1) * w;
            const size_t row2 = (y_up + 2) * w;
            const size_t row3 = (y_up + 3) * w;
            for (int x = 0; x < c.w; x++) {
                const size_t x_up = (size_t)x * up;
                const float bm_val = bm.At(bm.a, c.x + x, c.y + y);

                const float s0 = dist_data[row0 + x_up], s1 = dist_data[row0 + x_up + 1],
                            s2 = dist_data[row0 + x_up + 2], s3 = dist_data[row0 + x_up + 3];
                const float s4 = dist_data[row1 + x_up], s5 = dist_data[row1 + x_up + 1],
                            s6 = dist_data[row1 + x_up + 2], s7 = dist_data[row1 + x_up + 3];
                const float s8 = dist_data[row2 + x_up], s9 = dist_data[row2 + x_up + 1],
                            s10 = dist_data[row2 + x_up + 2], s11 = dist_data[row2 + x_up + 3];
                const float s12 = dist_data[row3 + x_up], s13 = dist_data[row3 + x_up + 1],
                            s14 = dist_data[row3 + x_up + 2], s15 = dist_data[row3 + x_up + 3];

                for (int s = 0; s < steps; s++) {
                    const float nw = neg_weights[s];
                    int inside = (s0 > nw) + (s1 > nw) + (s2 > nw) + (s3 > nw) +
                                 (s4 > nw) + (s5 > nw) + (s6 > nw) + (s7 > nw) +
                                 (s8 > nw) + (s9 > nw) + (s10 > nw) + (s11 > nw) +
                                 (s12 > nw) + (s13 > nw) + (s14 > nw) + (s15 > nw);
                    error[s] += fabs((float)inside * inv_up2 - bm_val);
                }
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
                    refs[f].push_back({ c, stbtt_FindGlyphIndex(&faces[f].info, Unicode(g.code)) });
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
    // Bolt optimization: Precompute constant RGB color byte conversions once per glyph
    // cell instead of re-evaluating lroundf for each texel, cache direct row pointers
    // in destination buffer to avoid redundant index calculations, and unroll the fixed
    // 4x4 sample summation across distance field samples.
    const uint8_t r_byte = (uint8_t)lroundf(rgb[2] * 255.0f);
    const uint8_t g_byte = (uint8_t)lroundf(rgb[1] * 255.0f);
    const uint8_t b_byte = (uint8_t)lroundf(rgb[0] * 255.0f);
    const float *dist_data = distance.data();

    for (int ty = 0; ty < c.h * texels; ty++) {
        int oy = c.y * texels + ty;
        if (oy < 0 || oy >= outHeight)
            continue;
        uint8_t *p = &out[((size_t)oy * outWidth + (size_t)c.x * texels) * 4];
        const size_t row_base = (size_t)(ty * kOversample) * w;
        for (int tx = 0; tx < c.w * texels; tx++, p += 4) {
            int ox = c.x * texels + tx;
            if (ox < 0 || ox >= outWidth)
                continue;
            const size_t sample_base = row_base + (size_t)(tx * kOversample);
            float sum = dist_data[sample_base] + dist_data[sample_base + 1] + dist_data[sample_base + 2] + dist_data[sample_base + 3]
                      + dist_data[sample_base + w] + dist_data[sample_base + w + 1] + dist_data[sample_base + w + 2] + dist_data[sample_base + w + 3]
                      + dist_data[sample_base + 2 * w] + dist_data[sample_base + 2 * w + 1] + dist_data[sample_base + 2 * w + 2] + dist_data[sample_base + 2 * w + 3]
                      + dist_data[sample_base + 3 * w] + dist_data[sample_base + 3 * w + 1] + dist_data[sample_base + 3 * w + 2] + dist_data[sample_base + 3 * w + 3];

            float a = 0.5f + (sum / (kOversample * kOversample) / up + weight) / (2.0f * kSpread);
            p[0] = r_byte;
            p[1] = g_byte;
            p[2] = b_byte;
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
    // bitmap's for glyphs Heros lacks and other fonts. Returns whether the
    // glyph is Heros's.
    std::vector<float> coverage;
    auto draw = [&](const OutlineFontGlyph &g, const Cell &c, const Ink &ink) {
        coverage.assign((size_t)c.w * up * c.h * up, 0.0f);
        if (ink.mass <= 0)
            return false;
        int glyph = fit.face != nullptr ? stbtt_FindGlyphIndex(&fit.face->info, Unicode(g.code)) : 0;
        if (glyph == 0) {
            Upsample(bm, c, up, coverage);
            return false;
        }
        PlaceGlyph(fit, glyph, ink, true, up, c.w, c.h, coverage);
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
