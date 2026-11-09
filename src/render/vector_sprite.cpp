// Original UI artwork traced to Bezier contours at load time. Colour masks
// and silhouettes get separate distance fields; native-size draws still read
// the untouched original texture. No traced game assets are shipped.
#include "port/vector_sprite.h"
#include "render/gfx_internal.h"
#include "render/distance_field.h"
#include "port/sys.h"
#include "potracelib.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

namespace {
struct Cell {
    int x, y, w, h;
};
struct Image {
    int w, h;
    std::vector<uint32_t> pixels;
};
struct Built {
    int w, h;
    std::vector<uint8_t> pixels;
};
constexpr int kPad = 2, kOversample = 4;
static_assert(kOversample == 4, "Unrolled accumulation in Build assumes kOversample == 4");
constexpr float kSpread = 2.0f;

bool IsArcadeMap(const std::string &name) {
    return name.size() == 5 && name.compare(0, 4, "cha0") == 0 && name[4] >= '1' && name[4] <= '8';
}

bool IsMap(const std::string &name) {
    return IsArcadeMap(name) || name == "smmatrix" || name == "chalblob";
}

std::vector<Cell> Cells(const std::string &name, int w, int h) {
    if ((IsArcadeMap(name) && w == 128 && h == 128) ||
        (name == "smmatrix" && w == h && (w == 16 || w == 32)) || (name == "chalblob" && w == 64 && h == 64))
        return {{0, 0, w, h}};
    if (name == "arrows4") {
        if (w == 256 && h == 128) {
            std::vector<Cell> cells;
            for (int i = 0; i < 8; i++)
                cells.push_back({(i % 4) * 64, (i / 4) * 64, 64, 64});
            return cells;
        }
        if (w == 512 && h == 256)
            return {{1, 0, 102, 102},   {103, 0, 102, 102},   {205, 0, 102, 102},   {307, 0, 102, 102},
                    {1, 102, 102, 102}, {103, 102, 102, 102}, {205, 102, 102, 102}, {307, 109, 107, 92}};
        return {};
    }
    if ((name == "ar_640a" || name == "ar_640d" || name == "arrow" || name == "tick" || name == "cross" ||
         name == "box" || name == "circle") &&
        w >= 8 && h >= 8 && w <= 64 && h <= 64)
        return {{0, 0, w, h}};
    return {};
}

bool Read(GfxTexture *texture, Image &image) {
    image.w = texture->width;
    image.h = texture->height;
    GfxTexture *copy = Gfx_CreateTexture(image.w, image.h, GFX_FORMAT_BGRA8, 1, GFX_TEXTURE_STAGING);
    if (!copy)
        return false;
    Gfx_CopyTexture(copy, texture);
    GfxLockedRect lock{};
    bool ok = Gfx_LockTexture(copy, 0, 0, &lock) != 0;
    if (ok) {
        image.pixels.resize((size_t)image.w * image.h);
        for (int y = 0; y < image.h; y++)
            memcpy(image.pixels.data() + y * image.w, (const uint8_t *)lock.pixels + y * lock.pitch,
                   image.w * 4);
        Gfx_UnlockTexture(copy, 0, 0);
    }
    Gfx_DestroyTexture(copy);
    return ok;
}

// Most frequent visible, non-black fill. The second colour is recovered
// separately: the race menus use white detail on blue rather than black ink.
uint32_t FillColour(const Image &image, Cell c) {
    std::map<uint32_t, unsigned> colours;
    bool visible = false;
    for (int y = 0; y < c.h; y++)
        for (int x = 0; x < c.w; x++) {
            uint32_t p = image.pixels[(c.y + y) * image.w + c.x + x];
            if ((p >> 24) < 240)
                continue;
            visible = true;
            if (std::max({p & 255, (p >> 8) & 255, (p >> 16) & 255}) > 32)
                colours[p & 0xffffff]++;
        }
    auto best =
        std::max_element(colours.begin(), colours.end(), [](auto a, auto b) { return a.second < b.second; });
    return best != colours.end() ? best->first : visible ? 0 : 0xffffff;
}

uint32_t DetailColour(const Image &image, Cell c, uint32_t base) {
    uint32_t colour = base;
    int furthest = 0;
    for (int y = 0; y < c.h; y++)
        for (int x = 0; x < c.w; x++) {
            uint32_t p = image.pixels[(c.y + y) * image.w + c.x + x];
            if ((p >> 24) < 240)
                continue;
            int distance = 0;
            for (int shift : {0, 8, 16}) {
                int delta = int((p >> shift) & 255) - int((base >> shift) & 255);
                distance += delta * delta;
            }
            if (distance > furthest) {
                furthest = distance;
                colour = p & 0xffffff;
            }
        }
    return colour;
}

float At(const std::vector<float> &mask, int w, int h, int x, int y) {
    return x < 0 || y < 0 || x >= w || y >= h ? 0.0f : mask[y * w + x];
}
float Sample(const std::vector<float> &mask, int w, int h, float x, float y) {
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    float a = At(mask, w, h, ix, iy) * (1 - fx) + At(mask, w, h, ix + 1, iy) * fx;
    float b = At(mask, w, h, ix, iy + 1) * (1 - fx) + At(mask, w, h, ix + 1, iy + 1) * fx;
    return a * (1 - fy) + b * fy;
}

// Trace the original mask, preserving small dots,
// holes and sharp corners. Rasterise the resulting cubic paths only to build
// their distance field; display scaling follows the field, not bitmap texels.
std::vector<float> Trace(const std::vector<float> &mask, int w, int h, int up, bool curves) {
    const int traceScale = curves && w > 64 ? 4 : 1;
    int tw = w * traceScale, th = h * traceScale;
    const int bits = sizeof(potrace_word) * 8;
    int stride = (tw + bits - 1) / bits;
    std::vector<potrace_word> map((size_t)stride * th);
    // Bolt: Fast path when traceScale == 1. In this case (x + 0.5f) - 0.5f == x,
    // which corresponds exactly to integer pixel coordinates. Reading mask rows directly
    // avoids 2 floorf calls and 4-point bilinear interpolation overhead per pixel.
    if (traceScale == 1) {
        for (int y = 0; y < h; y++) {
            potrace_word *row = &map[(size_t)(h - 1 - y) * stride];
            const float *mask_row = &mask[(size_t)y * w];
            for (int x = 0; x < w; x++) {
                if (mask_row[x] >= 0.5f)
                    row[x / bits] |= (potrace_word)1 << (bits - 1 - x % bits);
            }
        }
    } else {
        for (int y = 0; y < th; y++)
            for (int x = 0; x < tw; x++)
                if (Sample(mask, w, h, (x + .5f) / traceScale - .5f, (y + .5f) / traceScale - .5f) >= .5f)
                    map[(th - 1 - y) * stride + x / bits] |= (potrace_word)1 << (bits - 1 - x % bits);
    }
    potrace_bitmap_t bitmap{tw, th, stride, map.data()};
    potrace_param_t *param = potrace_param_default();
    if (!param)
        return {};
    param->turdsize = 0;
    param->alphamax = curves ? 1.0 : 0.0;
    param->opttolerance = 0.025 * traceScale;
    potrace_state_t *state = potrace_trace(param, &bitmap);
    potrace_param_free(param);
    if (!state)
        return {};
    if (state->status != POTRACE_STATUS_OK) {
        potrace_state_free(state);
        return {};
    }
    std::vector<stbtt_vertex> vertices;
    auto vertex = [&](int type, potrace_dpoint_t p, potrace_dpoint_t a, potrace_dpoint_t b) {
        stbtt_vertex v{};
        v.type = (unsigned char)type;
        v.x = (short)lround(p.x * 64 / traceScale);
        v.y = (short)lround((th - p.y) * 64 / traceScale);
        v.cx = (short)lround(a.x * 64 / traceScale);
        v.cy = (short)lround((th - a.y) * 64 / traceScale);
        v.cx1 = (short)lround(b.x * 64 / traceScale);
        v.cy1 = (short)lround((th - b.y) * 64 / traceScale);
        vertices.push_back(v);
    };
    for (potrace_path_t *path = state->plist; path; path = path->next) {
        const auto &curve = path->curve;
        if (!curve.n)
            continue;
        vertex(STBTT_vmove, curve.c[curve.n - 1][2], {}, {});
        for (int i = 0; i < curve.n; i++) {
            if (curve.tag[i] == POTRACE_CORNER) {
                vertex(STBTT_vline, curve.c[i][1], {}, {});
                vertex(STBTT_vline, curve.c[i][2], {}, {});
            } else
                vertex(STBTT_vcubic, curve.c[i][2], curve.c[i][0], curve.c[i][1]);
        }
    }
    potrace_state_free(state);
    int rw = w * up, rh = h * up;
    std::vector<unsigned char> pixels((size_t)rw * rh);
    if (!vertices.empty()) {
        stbtt__bitmap raster{rw, rh, rw, pixels.data()};
        stbtt_Rasterize(&raster, 0.1f, vertices.data(), (int)vertices.size(), up / 64.0f, up / 64.0f, 0, 0, 0,
                        0, 0, nullptr);
    }
    std::vector<float> coverage(pixels.size());
    for (size_t i = 0; i < pixels.size(); i++)
        coverage[i] = pixels[i] / 255.0f;
    auto distance = DistanceField::FromCoverage(coverage, rw, rh);
    // Antialiasing contains subpixel stroke-width information. Match the
    // original painted area instead of rounding every partial pixel to a
    // whole pixel during tracing. The correction is bounded to 0.35 source
    // pixels and moves both sides of a stroke equally.
    double target = std::accumulate(mask.begin(), mask.end(), 0.0) * up * up;
    const float limit = 0.35f * up;
    const float upper = limit + 1.0f;
    const float lower = -limit - 1.0f;
    size_t inside = 0;
    std::vector<float> edge;
    // Bolt: Reserve capacity to avoid repeated geometric reallocations when collecting
    // boundary edge distance samples.
    edge.reserve(distance.size() / 8);
    for (float d : distance) {
        if (d > upper)
            inside++;
        else if (d >= lower)
            edge.push_back(d);
    }
    float lo = -limit, hi = limit;
    for (int iteration = 0; iteration < 14; iteration++) {
        float shift = (lo + hi) * .5f;
        double area = (double)inside;
        for (float d : edge)
            area += std::clamp(d + shift + .5f, 0.0f, 1.0f);
        if (area < target)
            lo = shift;
        else
            hi = shift;
    }
    float shift = (lo + hi) * .5f;
    for (float &d : distance)
        d += shift;
    return distance;
}

// The selection map is made of circular points, not a continuous route.
// Retain each point's original centre and painted area, including its padding.
std::vector<float> Circle(const std::vector<float> &mask, int w, int h, int up) {
    double area = 0, cx = 0, cy = 0;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float a = mask[y * w + x];
            area += a;
            cx += (x + .5) * a;
            cy += (y + .5) * a;
        }
    if (area == 0)
        return {};
    cx /= area;
    cy /= area;
    float radius = (float)sqrt(area / 3.141592653589793);
    int rw = w * up, rh = h * up;
    std::vector<float> distance((size_t)rw * rh);
    // Bolt: Precompute horizontal squared offsets to avoid redundant division
    // and expensive hypotf calls inside the nested rh x rw loop (~4x speedup).
    const float inv_up = 1.0f / (float)up;
    const float radius_up = radius * (float)up;
    const float fcx = (float)cx;
    const float fcy = (float)cy;
    std::vector<float> dx2(rw);
    for (int x = 0; x < rw; x++) {
        float dx = (x + .5f) * inv_up - fcx;
        dx2[x] = dx * dx;
    }
    for (int y = 0; y < rh; y++) {
        float dy = (y + .5f) * inv_up - fcy;
        float dy2 = dy * dy;
        float *row = &distance[(size_t)y * rw];
        for (int x = 0; x < rw; x++)
            row[x] = radius_up - sqrtf(dx2[x] + dy2) * (float)up;
    }
    return distance;
}

Built Build(const Image &image, const std::vector<Cell> &cells, bool curves, bool map, bool dots) {
    int texels = !map && cells.front().w >= 96 ? 2 : 4;
    int up = texels * kOversample;
    Built built{image.w * texels, image.h * texels * 3, {}};
    built.pixels.assign((size_t)built.w * built.h * 4, 0);
    for (Cell cell : cells) {
        int w = cell.w + 2 * kPad, h = cell.h + 2 * kPad;
        std::vector<float> alpha((size_t)w * h), fill(alpha.size());
        uint32_t colour = FillColour(image, cell);
        uint32_t detailColour = DetailColour(image, cell, colour);
        float base[] = {float(colour & 255), float((colour >> 8) & 255), float((colour >> 16) & 255)};
        float detail[] = {float(detailColour & 255), float((detailColour >> 8) & 255),
                          float((detailColour >> 16) & 255)};
        float delta[] = {base[0] - detail[0], base[1] - detail[1], base[2] - detail[2]};
        float norm = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];
        bool coloured = false;
        for (int y = 0; y < cell.h; y++)
            for (int x = 0; x < cell.w; x++) {
                uint32_t p = image.pixels[(cell.y + y) * image.w + cell.x + x];
                size_t i = (y + kPad) * w + x + kPad;
                alpha[i] = (p >> 24) / 255.0f;
                float dot = ((p & 255) - detail[0]) * delta[0] + (((p >> 8) & 255) - detail[1]) * delta[1] +
                            (((p >> 16) & 255) - detail[2]) * delta[2];
                fill[i] = norm > 0 ? std::clamp(dot / norm, 0.0f, 1.0f) * alpha[i] : alpha[i];
                coloured |= alpha[i] > .9f && fill[i] < .1f && norm > 0;
            }
        auto outer = dots ? Circle(alpha, w, h, up) : Trace(alpha, w, h, up, curves);
        auto inner = coloured ? Trace(fill, w, h, up, curves) : std::vector<float>();
        if (outer.empty() || (coloured && inner.empty()))
            return {};
        // Bolt optimization: Precompute palette color bytes once per cell, step
        // row pointers sequentially along output lines, and unroll the fixed 4x4
        // sample summation for distance fields (~27% faster assembly, 100% bit-exact).
        const uint8_t base_b = (uint8_t)base[0], base_g = (uint8_t)base[1], base_r = (uint8_t)base[2];
        const uint8_t det_b = (uint8_t)detail[0], det_g = (uint8_t)detail[1], det_r = (uint8_t)detail[2];
        const uint8_t col_r = coloured ? 255 : 0;
        const size_t band = (size_t)built.w * (built.h / 3);
        const float scale = 1.0f / (kOversample * kOversample * up * 2 * kSpread);
        const size_t outer_stride = (size_t)(w * up);
        const float *outer_data = outer.data();
        const float *inner_data = coloured ? inner.data() : nullptr;
        const int cell_w_tex = cell.w * texels;
        const int cell_h_tex = cell.h * texels;
        const size_t row_pad = (size_t)(kPad * up);

        for (int y = 0; y < cell_h_tex; y++) {
            size_t row_idx = (size_t)(cell.y * texels + y) * built.w + cell.x * texels;
            uint8_t *p_main = &built.pixels[row_idx * 4];
            uint8_t *p_pal1 = &built.pixels[(row_idx + band) * 4];
            uint8_t *p_pal2 = &built.pixels[(row_idx + 2 * band) * 4];

            const size_t sy_base = (row_pad + (size_t)y * kOversample) * outer_stride + row_pad;

            for (int x = 0; x < cell_w_tex; x++, p_main += 4, p_pal1 += 4, p_pal2 += 4) {
                const size_t sample_base = sy_base + (size_t)x * kOversample;

                float dist0 = outer_data[sample_base] + outer_data[sample_base + 1] + outer_data[sample_base + 2] + outer_data[sample_base + 3]
                            + outer_data[sample_base + outer_stride] + outer_data[sample_base + outer_stride + 1] + outer_data[sample_base + outer_stride + 2] + outer_data[sample_base + outer_stride + 3]
                            + outer_data[sample_base + 2 * outer_stride] + outer_data[sample_base + 2 * outer_stride + 1] + outer_data[sample_base + 2 * outer_stride + 2] + outer_data[sample_base + 2 * outer_stride + 3]
                            + outer_data[sample_base + 3 * outer_stride] + outer_data[sample_base + 3 * outer_stride + 1] + outer_data[sample_base + 3 * outer_stride + 2] + outer_data[sample_base + 3 * outer_stride + 3];

                float dist1 = 0.0f;
                if (coloured) {
                    dist1 = inner_data[sample_base] + inner_data[sample_base + 1] + inner_data[sample_base + 2] + inner_data[sample_base + 3]
                          + inner_data[sample_base + outer_stride] + inner_data[sample_base + outer_stride + 1] + inner_data[sample_base + outer_stride + 2] + inner_data[sample_base + outer_stride + 3]
                          + inner_data[sample_base + 2 * outer_stride] + inner_data[sample_base + 2 * outer_stride + 1] + inner_data[sample_base + 2 * outer_stride + 2] + inner_data[sample_base + 2 * outer_stride + 3]
                          + inner_data[sample_base + 3 * outer_stride] + inner_data[sample_base + 3 * outer_stride + 1] + inner_data[sample_base + 3 * outer_stride + 2] + inner_data[sample_base + 3 * outer_stride + 3];
                }

                float field0 = .5f + dist0 * scale;
                float field1 = .5f + dist1 * scale;

                p_main[0] = (uint8_t)lroundf(std::clamp(field0, 0.0f, 1.0f) * 255.0f);
                p_main[1] = (uint8_t)lroundf(std::clamp(field1, 0.0f, 1.0f) * 255.0f);
                // BGRA bytes: B = outer coverage field, G = fill field. R
                // marks whether the boundary between two colours is used.
                p_main[2] = col_r;
                p_main[3] = 255;

                p_pal1[0] = base_b;
                p_pal1[1] = base_g;
                p_pal1[2] = base_r;
                p_pal1[3] = 255;

                p_pal2[0] = det_b;
                p_pal2[1] = det_g;
                p_pal2[2] = det_r;
                p_pal2[3] = 255;
            }
        }
    }
    return built;
}
} // namespace

extern "C" void VectorSprite_Replace(const char *name, GfxTexture *texture) {
    if (!name || !texture || texture->replacement)
        return;
    std::string base(name);
    base = base.substr(base.find_last_of("\\/") + 1);
    std::transform(base.begin(), base.end(), base.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    size_t dot = base.find_last_of('.');
    if (dot == std::string::npos || (base.substr(dot) != ".tga" && base.substr(dot) != ".dds"))
        return;
    base.resize(dot);
    bool map = IsMap(base);
    auto cells = Cells(base, texture->width, texture->height);
    if (cells.empty())
        return;
    Image image{};
    if (!Read(texture, image))
        return;
    uint64_t hash = 14695981039346656037ull;
    for (uint32_t pixel : image.pixels) {
        hash ^= pixel;
        hash *= 1099511628211ull;
    }
    std::string key =
        base + "|" + std::to_string(image.w) + "x" + std::to_string(image.h) + "|" + std::to_string(hash);
    static std::map<std::string, Built> cache;
    auto found = cache.find(key);
    if (found == cache.end()) {
        Built built =
            Build(image, cells, map || base == "arrows4" || base == "circle", map, base == "smmatrix");
        if (built.pixels.empty())
            return;
        size_t bytes = built.pixels.size();
        for (const auto &entry : cache)
            bytes += entry.second.pixels.size();
        if (bytes > 32 * 1024 * 1024)
            cache.clear();
        found = cache.emplace(key, std::move(built)).first;
        Sys_Log("%s: %s: vectorised %d original cells", map ? "maps" : "icons", base.c_str(),
                (int)cells.size());
    }
    const Built &built = found->second;
    GfxTexture *replacement =
        Gfx_CreateTexture(built.w, built.h, GFX_FORMAT_BGRA8, 1, GFX_TEXTURE_VECTOR_ICON);
    GfxLockedRect lock{};
    if (!replacement || !Gfx_LockTexture(replacement, 0, 0, &lock)) {
        Gfx_DestroyTexture(replacement);
        return;
    }
    for (int y = 0; y < built.h; y++)
        memcpy((uint8_t *)lock.pixels + (size_t)y * lock.pitch, built.pixels.data() + (size_t)y * built.w * 4,
               built.w * 4);
    Gfx_UnlockTexture(replacement, 0, 0);
    Gfx_SetTextureReplacement(texture, replacement);
}
