// The SDL_GPU executor: runs a recorded frame (gfx_core.cpp) with one set of
// shaders that emulate Direct3D 7's fixed-function pipeline.

#include "platform/platform.h"
#include "port/sys.h"
#include "render/presentation.h"
#include "render/gfx_internal.h"

#include "shaders_spirv.h"

// SPIR-V and, when built, MSL code of one shader for Executor::Shader.
#ifdef OPENCMR2_SHADERS_MSL
#define SHADER_CODE(name) g_##name, sizeof(g_##name), g_##name##_msl, sizeof(g_##name##_msl)
#else
#define SHADER_CODE(name) g_##name, sizeof(g_##name), nullptr, 0
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <unordered_map>

namespace {

// ---- uniform blocks (std140, see shaders/common.glsl and fixed.frag) ----

struct LightUniform {
    float diffuse[4], specular[4], ambient[4], position[4], direction[4], attenuation[4], spot[4];
};

struct VertexUniforms {
    float worldView[16];
    float projection[16];
    float textureMatrix[2][16];
    float materialDiffuse[4], materialAmbient[4], materialSpecular[4], materialEmissive[4];
    float globalAmbient[4];
    LightUniform lights[8];
    int32_t lightFlags[4];
    int32_t sources[4];
    int32_t fogFlags[4];
    float fogParams[4];
    int32_t texCoordIndex[4];
    float target[4];
};

struct FragmentUniforms {
    int32_t stage0[4], stage0b[4], stage1[4], stage1b[4];
    float textureFactor[4];
    int32_t alphaTest[4];
    float alphaRef[4];
    float fogColor[4];
    int32_t fogMode[4];
    float fogParams[4];
    float bumpMatrix[4];
    float bumpLuminance[4];
    float distanceFade[4];
};

struct ClearUniforms {
    float color[4];
    float depth[4];
};

void Multiply(const GfxMatrix &a, const GfxMatrix &b, float *out)
{
    const float *pa = &a._11, *pb = &b._11;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            float s = 0;
            for (int k = 0; k < 4; k++)
                s += pa[r * 4 + k] * pb[k * 4 + c];
            out[r * 4 + c] = s;
        }
}

void TransformPoint(const GfxVector &v, float w, const GfxMatrix &m, float out[3])
{
    const float *p = &m._11;
    for (int c = 0; c < 3; c++)
        out[c] = v.x * p[c] + v.y * p[4 + c] + v.z * p[8 + c] + w * p[12 + c];
}

void Color(DWORD c, float out[4])
{
    out[0] = ((c >> 16) & 0xff) / 255.0f;
    out[1] = ((c >> 8) & 0xff) / 255.0f;
    out[2] = (c & 0xff) / 255.0f;
    out[3] = ((c >> 24) & 0xff) / 255.0f;
}

void Copy4(const GfxColorValue &c, float out[4])
{
    out[0] = c.r;
    out[1] = c.g;
    out[2] = c.b;
    out[3] = c.a;
}

SDL_GPUBlendFactor BlendFactor(int d3d)
{
    switch (d3d) {
    case GFX_BLEND_ZERO: return SDL_GPU_BLENDFACTOR_ZERO;
    case GFX_BLEND_ONE: return SDL_GPU_BLENDFACTOR_ONE;
    case GFX_BLEND_SRCCOLOR: return SDL_GPU_BLENDFACTOR_SRC_COLOR;
    case GFX_BLEND_INVSRCCOLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
    case GFX_BLEND_SRCALPHA: return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case GFX_BLEND_INVSRCALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case GFX_BLEND_DESTALPHA: return SDL_GPU_BLENDFACTOR_DST_ALPHA;
    case GFX_BLEND_INVDESTALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
    case GFX_BLEND_DESTCOLOR: return SDL_GPU_BLENDFACTOR_DST_COLOR;
    case GFX_BLEND_INVDESTCOLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
    case GFX_BLEND_SRCALPHASAT: return SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE;
    default: return SDL_GPU_BLENDFACTOR_ONE;
    }
}

SDL_GPUCompareOp CompareOp(int d3d)
{
    switch (d3d) {
    case GFX_CMP_NEVER: return SDL_GPU_COMPAREOP_NEVER;
    case GFX_CMP_LESS: return SDL_GPU_COMPAREOP_LESS;
    case GFX_CMP_EQUAL: return SDL_GPU_COMPAREOP_EQUAL;
    case GFX_CMP_LESSEQUAL: return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    case GFX_CMP_GREATER: return SDL_GPU_COMPAREOP_GREATER;
    case GFX_CMP_NOTEQUAL: return SDL_GPU_COMPAREOP_NOT_EQUAL;
    case GFX_CMP_GREATEREQUAL: return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
    default: return SDL_GPU_COMPAREOP_ALWAYS;
    }
}

SDL_GPUPrimitiveType PrimitiveType(int d3d)
{
    switch (d3d) {
    case GFX_POINTLIST: return SDL_GPU_PRIMITIVETYPE_POINTLIST;
    case GFX_LINELIST: return SDL_GPU_PRIMITIVETYPE_LINELIST;
    case GFX_LINESTRIP: return SDL_GPU_PRIMITIVETYPE_LINESTRIP;
    case GFX_TRIANGLESTRIP: return SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
    default: return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    }
}

struct GpuTexture {
    SDL_GPUTexture *texture = nullptr;
    SDL_GPUTexture *depth = nullptr;       // render targets
    SDL_GPUTextureFormat format;
    bool cpuDecode = false;                // BC data decoded on the CPU (no BC support)
};

// The GPU copy of a GfxVertexBuffer, as of a version of it.
struct GpuVertexBuffer {
    SDL_GPUBuffer *buffer = nullptr;
    uint32_t capacity = 0;
    uint32_t version = 0;
};

// The device the renderer asks SDL_GPU for: its shader formats, and no
// optional feature it does not use.
SDL_PropertiesID DeviceProperties()
{
    SDL_GPUShaderFormat formats = SDL_GPU_SHADERFORMAT_SPIRV;
#ifdef OPENCMR2_SHADERS_MSL
    formats |= SDL_GPU_SHADERFORMAT_MSL;
#endif
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN,
                           (formats & SDL_GPU_SHADERFORMAT_SPIRV) != 0);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN,
                           (formats & SDL_GPU_SHADERFORMAT_MSL) != 0);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN,
                           Platform_GetSettingInt("video.gpu_debug", 0) != 0);
    // SDL asks Vulkan for these features by default. The renderer does not
    // use them (anisotropy only when video.anisotropy is set, see Sampler),
    // and many Android GPUs lack some, for which SDL would refuse the device.
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN,
                           Platform_GetSettingInt("video.anisotropy", 1) > 1);
    return props;
}

struct Executor : GfxExecutor {
    SDL_GPUDevice *device = nullptr;
    SDL_Window *window = nullptr;
    SDL_GPUShader *vs3d = nullptr, *vsTL = nullptr, *fs = nullptr, *vsClear = nullptr, *fsClear = nullptr;
    SDL_GPUTexture *backColor = nullptr, *backDepth = nullptr;
    SDL_GPUTexture *backdrop = nullptr;    // the copy of backColor the distance fade reads
    int backWidth = 0, backHeight = 0;
    SDL_GPUTextureFormat colorFormat = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    SDL_GPUTextureFormat depthFormat = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
    SDL_GPUBuffer *vertexBuffer = nullptr, *indexBuffer = nullptr;
    uint32_t vertexCapacity = 0, indexCapacity = 0;
    SDL_GPUTransferBuffer *uploadBuffer = nullptr;  // the frame's vertices and indices
    uint32_t uploadCapacity = 0;
    SDL_GPUTexture *white2D = nullptr, *whiteCube = nullptr;
    std::unordered_map<uint32_t, SDL_GPUGraphicsPipeline *> pipelines;
    std::unordered_map<uint32_t, SDL_GPUSampler *> samplers;
    bool bcSupported = true;
    bool presentModeVsync = true;
    int anisotropy = 1, smoothScaling = 1;    // video.anisotropy, video.smooth_scaling
    Uint32 presentWidth = 0, presentHeight = 0; // the window's pixels, as last logged
    // The back buffer read of the last presented frame (the lens flare test):
    // it runs with that frame, and CollectReadback waits for it only when the
    // game reads the pixels, during the next frame.
    SDL_GPUTransferBuffer *readbackBuffer = nullptr;
    uint32_t readbackCapacity = 0;
    SDL_GPUFence *readbackFence = nullptr;
    int readbackWidth = 0, readbackHeight = 0;

    ~Executor() override
    {
        if (device == NULL)
            return;
        SDL_WaitForGPUIdle(device);
        SDL_ReleaseGPUFence(device, readbackFence);
        SDL_ReleaseGPUTransferBuffer(device, readbackBuffer);
        for (auto &p : pipelines)
            SDL_ReleaseGPUGraphicsPipeline(device, p.second);
        for (auto &s : samplers)
            SDL_ReleaseGPUSampler(device, s.second);
        SDL_ReleaseGPUTexture(device, backColor);
        SDL_ReleaseGPUTexture(device, backDepth);
        SDL_ReleaseGPUTexture(device, backdrop);
        SDL_ReleaseGPUTexture(device, white2D);
        SDL_ReleaseGPUTexture(device, whiteCube);
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_ReleaseGPUTransferBuffer(device, uploadBuffer);
        SDL_ReleaseGPUShader(device, vs3d);
        SDL_ReleaseGPUShader(device, vsTL);
        SDL_ReleaseGPUShader(device, fs);
        SDL_ReleaseGPUShader(device, vsClear);
        SDL_ReleaseGPUShader(device, fsClear);
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
    }

    // The shader format the device takes: SPIR-V (Vulkan) or MSL (Metal).
    SDL_GPUShaderFormat shaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;

    SDL_GPUShader *Shader(const unsigned char *spirv, size_t spirvSize, const unsigned char *msl, size_t mslSize,
                          SDL_GPUShaderStage stage, int samplers, int uniforms)
    {
        SDL_GPUShaderCreateInfo info = {};
        if (shaderFormat == SDL_GPU_SHADERFORMAT_MSL) {
            info.code = msl;
            info.code_size = mslSize - 1; // without the NUL the embedding adds
            info.entrypoint = "main0";    // SPIRV-Cross renames main for MSL
        } else {
            info.code = spirv;
            info.code_size = spirvSize;
            info.entrypoint = "main";
        }
        info.format = shaderFormat;
        info.stage = stage;
        info.num_samplers = samplers;
        info.num_uniform_buffers = uniforms;
        SDL_GPUShader *shader = SDL_CreateGPUShader(device, &info);
        if (shader == NULL)
            Sys_Log("gfx: shader: %s", SDL_GetError());
        return shader;
    }

    SDL_GPUTexture *CreateTexture(SDL_GPUTextureType type, SDL_GPUTextureFormat format, int w, int h, int layers,
                                  int levels, SDL_GPUTextureUsageFlags usage)
    {
        SDL_GPUTextureCreateInfo info = {};
        info.type = type;
        info.format = format;
        info.usage = usage;
        info.width = w;
        info.height = h;
        info.layer_count_or_depth = layers;
        info.num_levels = levels;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        SDL_GPUTexture *t = SDL_CreateGPUTexture(device, &info);
        if (t == NULL)
            Sys_Log("gfx: texture %dx%d: %s", w, h, SDL_GetError());
        return t;
    }

    bool CreateDevice()
    {
        window = Platform_GetWindow();
        if (window == NULL) {
            Sys_Log("gfx: no window");
            return false;
        }
        anisotropy = Platform_GetSettingInt("video.anisotropy", 1);
        smoothScaling = Platform_GetSettingInt("video.smooth_scaling", 1);
        SDL_PropertiesID props = DeviceProperties();
        device = SDL_CreateGPUDeviceWithProperties(props);
        SDL_DestroyProperties(props);
        if (device == NULL) {
            Sys_Log("gfx: SDL_CreateGPUDevice: %s", SDL_GetError());
            return false;
        }
        if (!(SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV))
            shaderFormat = SDL_GPU_SHADERFORMAT_MSL;
        if (!SDL_ClaimWindowForGPUDevice(device, window)) {
            Sys_Log("gfx: SDL_ClaimWindowForGPUDevice: %s", SDL_GetError());
            return false;
        }
        Sys_Log("gfx: SDL_GPU driver %s", SDL_GetGPUDeviceDriver(device));
        if (!SDL_GPUTextureSupportsFormat(device, depthFormat, SDL_GPU_TEXTURETYPE_2D,
                                          SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
            depthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        bcSupported = SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM,
                                                   SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_SAMPLER);

        vs3d = Shader(SHADER_CODE(fixed3d_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        vsTL = Shader(SHADER_CODE(fixedtl_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        fs = Shader(SHADER_CODE(fixed_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 5, 1);
        vsClear = Shader(SHADER_CODE(clear_vert), SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        fsClear = Shader(SHADER_CODE(clear_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);
        if (!vs3d || !vsTL || !fs || !vsClear || !fsClear)
            return false;

        white2D = CreateTexture(SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM, 1, 1, 1, 1,
                                SDL_GPU_TEXTUREUSAGE_SAMPLER);
        whiteCube = CreateTexture(SDL_GPU_TEXTURETYPE_CUBE, SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM, 1, 1, 6, 1,
                                  SDL_GPU_TEXTUREUSAGE_SAMPLER);
        uint32_t white[6] = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff };
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, sizeof(white), 0 };
        SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
        memcpy(SDL_MapGPUTransferBuffer(device, tb, false), white, sizeof(white));
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        for (int layer = 0; layer < 7; layer++) {
            SDL_GPUTextureTransferInfo src = { tb, (Uint32)(layer < 6 ? layer * 4 : 0), 0, 0 };
            SDL_GPUTextureRegion dst = {};
            dst.texture = layer < 6 ? whiteCube : white2D;
            dst.layer = layer < 6 ? layer : 0;
            dst.w = dst.h = dst.d = 1;
            SDL_UploadToGPUTexture(copy, &src, &dst, false);
        }
        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(device, tb);

        // The pipelines the game draws with (keys of PipelineKey: menus,
        // stages, replays, results), made now: the first use of each could
        // take a frame's time to compile when the driver has not cached it.
        static const uint32_t common[] = {
            0x001a0459, 0x001a045b, 0x001a04b9, 0x001a0cb8, 0x001a0cb9, 0x001a0cbb, 0x001a2458, 0x001a2cb8,
            0x001a64b8, 0x001a6ca8, 0x001a6cb4, 0x001a6cb8, 0x000a2458, 0x000a2cb8, 0x000a6ca8, 0x000a6cb8,
        };
        for (uint32_t key : common)
            Pipeline(key);
        for (int clear = 1; clear < 4; clear++)
            ClearPipeline((clear & 1) != 0, (clear & 2) != 0);
        return true;
    }

    bool SetVideoMode(int width, int height, bool fullscreen) override
    {
        if (device == NULL && !CreateDevice())
            return false;
        if (backColor == NULL || backWidth != width || backHeight != height) {
            SDL_WaitForGPUIdle(device);
            SDL_ReleaseGPUTexture(device, backColor);
            SDL_ReleaseGPUTexture(device, backDepth);
            SDL_ReleaseGPUTexture(device, backdrop);
            backColor = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                      SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
            backDepth = CreateTexture(SDL_GPU_TEXTURETYPE_2D, depthFormat, width, height, 1, 1,
                                      SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);
            backdrop = CreateTexture(SDL_GPU_TEXTURETYPE_2D, colorFormat, width, height, 1, 1,
                                     SDL_GPU_TEXTUREUSAGE_SAMPLER);
            backWidth = width;
            backHeight = height;
            if (backColor == NULL || backDepth == NULL || backdrop == NULL)
                return false;
        }
        SDL_SetWindowResizable(window, true);
        SetFullscreen(fullscreen);
        SDL_ShowWindow(window);
        return true;
    }

    void SetFullscreen(bool fullscreen) override
    {
        Gfx_SetWindowMode(window, fullscreen, backWidth, backHeight);
    }

    // ---- textures ----

    SDL_GPUTextureFormat GpuFormat(int format, bool &cpuDecode)
    {
        cpuDecode = false;
        switch (format) {
        case GFX_FORMAT_BC1:
            if (bcSupported)
                return SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM;
            cpuDecode = true;
            return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        case GFX_FORMAT_BC3:
            if (bcSupported)
                return SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM;
            cpuDecode = true;
            return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        case GFX_FORMAT_BUMP_DUDVL:
            return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        default:
            return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        }
    }

    GpuTexture *Ensure(GfxTexture *t)
    {
        if (t->gpu != NULL)
            return (GpuTexture *)t->gpu;
        GpuTexture *g = new GpuTexture;
        g->format = GpuFormat(t->format, g->cpuDecode);
        bool cube = (t->flags & GFX_TEXTURE_CUBE) != 0;
        bool target = (t->flags & GFX_TEXTURE_RENDER_TARGET) != 0;
        SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        if (target)
            usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        g->texture = CreateTexture(cube ? SDL_GPU_TEXTURETYPE_CUBE : SDL_GPU_TEXTURETYPE_2D, g->format, t->width,
                                   t->height, cube ? 6 : 1, t->mipLevels, usage);
        if (target)
            g->depth = CreateTexture(SDL_GPU_TEXTURETYPE_2D, depthFormat, t->width, t->height, 1, 1,
                                     SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);
        t->gpu = g;
        t->dirty = true;
        return g;
    }

    // Bytes of a level as uploaded (after conversion).
    size_t UploadSize(GfxTexture *t, GpuTexture *g, int w, int h)
    {
        if ((t->format == GFX_FORMAT_BC1 || t->format == GFX_FORMAT_BC3) && !g->cpuDecode)
            return Gfx_LevelSize(t->format, w, h, NULL);
        return (size_t)w * h * 4;
    }

    void ConvertLevel(GfxTexture *t, GpuTexture *g, const uint8_t *src, int w, int h, uint8_t *dst)
    {
        size_t n = (size_t)w * h;
        switch (t->format) {
        case GFX_FORMAT_BGRX8:
            for (size_t i = 0; i < n; i++)
                ((uint32_t *)dst)[i] = ((const uint32_t *)src)[i] | 0xff000000;
            break;
        case GFX_FORMAT_BUMP_DUDVL:
            for (size_t i = 0; i < n; i++) {
                dst[i * 4] = src[i * 3];
                dst[i * 4 + 1] = src[i * 3 + 1];
                dst[i * 4 + 2] = src[i * 3 + 2];
                dst[i * 4 + 3] = 0xff;
            }
            break;
        case GFX_FORMAT_BC1:
        case GFX_FORMAT_BC3:
            if (g->cpuDecode) {
                int blockBytes = t->format == GFX_FORMAT_BC1 ? 8 : 16;
                int bw = (w + 3) / 4, bh = (h + 3) / 4;
                uint32_t block[16];
                for (int by = 0; by < bh; by++)
                    for (int bx = 0; bx < bw; bx++) {
                        Gfx_DecodeBlock(t->format, src + (by * bw + bx) * blockBytes, block);
                        for (int y = 0; y < 4; y++)
                            for (int x = 0; x < 4; x++)
                                if (bx * 4 + x < w && by * 4 + y < h)
                                    ((uint32_t *)dst)[(size_t)(by * 4 + y) * w + bx * 4 + x] = block[y * 4 + x];
                    }
            } else {
                memcpy(dst, src, Gfx_LevelSize(t->format, w, h, NULL));
            }
            break;
        default:
            memcpy(dst, src, n * 4);
            break;
        }
    }

    void UploadTextures(SDL_GPUCommandBuffer *cmd, std::vector<GfxTexture *> &textures)
    {
        size_t total = 0;
        for (GfxTexture *t : textures) {
            if (!t->dirty)
                continue;
            GpuTexture *g = Ensure(t);
            if (g->texture == NULL) {
                t->dirty = false;
                continue;
            }
            int faces = (t->flags & GFX_TEXTURE_CUBE) ? 6 : 1;
            for (int f = 0; f < faces; f++)
                for (int m = 0; m < t->mipLevels; m++)
                    total += UploadSize(t, g, std::max(1, t->width >> m), std::max(1, t->height >> m));
        }
        if (total == 0)
            return;
        SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, (Uint32)total, 0 };
        SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
        uint8_t *map = (uint8_t *)SDL_MapGPUTransferBuffer(device, tb, false);
        struct Pending {
            SDL_GPUTexture *texture;
            int face, level, w, h;
            size_t offset;
        };
        std::vector<Pending> pending;
        size_t offset = 0;
        for (GfxTexture *t : textures) {
            if (!t->dirty)
                continue;
            GpuTexture *g = (GpuTexture *)t->gpu;
            int faces = (t->flags & GFX_TEXTURE_CUBE) ? 6 : 1;
            for (int f = 0; f < faces; f++) {
                for (int m = 0; m < t->mipLevels; m++) {
                    int w = std::max(1, t->width >> m), h = std::max(1, t->height >> m);
                    ConvertLevel(t, g, t->levels[f][m].data(), w, h, map + offset);
                    pending.push_back({ g->texture, f, m, w, h, offset });
                    offset += UploadSize(t, g, w, h);
                }
            }
            t->dirty = false;
        }
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        for (const Pending &p : pending) {
            SDL_GPUTextureTransferInfo src = { tb, (Uint32)p.offset, 0, 0 };
            SDL_GPUTextureRegion dst = {};
            dst.texture = p.texture;
            dst.mip_level = p.level;
            dst.layer = p.face;
            dst.w = p.w;
            dst.h = p.h;
            dst.d = 1;
            SDL_UploadToGPUTexture(copy, &src, &dst, false);
        }
        SDL_EndGPUCopyPass(copy);
        SDL_ReleaseGPUTransferBuffer(device, tb);
    }

    void DestroyTexture(GfxTexture *t) override
    {
        GpuTexture *g = (GpuTexture *)t->gpu;
        if (g == NULL || device == NULL)
            return;
        SDL_ReleaseGPUTexture(device, g->texture);
        SDL_ReleaseGPUTexture(device, g->depth);
        delete g;
        t->gpu = NULL;
    }

    void ReadTexture(GfxTexture *t) override
    {
        GpuTexture *g = (GpuTexture *)t->gpu;
        if (g == NULL || t->format != GFX_FORMAT_BGRX8 && t->format != GFX_FORMAT_BGRA8)
            return;
        int faces = (t->flags & GFX_TEXTURE_CUBE) ? 6 : 1;
        size_t level = (size_t)t->width * t->height * 4;
        SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, (Uint32)(level * faces), 0 };
        SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        for (int f = 0; f < faces; f++) {
            SDL_GPUTextureRegion src = {};
            src.texture = g->texture;
            src.layer = f;
            src.w = t->width;
            src.h = t->height;
            src.d = 1;
            SDL_GPUTextureTransferInfo dst = { tb, (Uint32)(level * f), 0, 0 };
            SDL_DownloadFromGPUTexture(copy, &src, &dst);
        }
        SDL_EndGPUCopyPass(copy);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        SDL_WaitForGPUFences(device, true, &fence, 1);
        SDL_ReleaseGPUFence(device, fence);
        const uint8_t *map = (const uint8_t *)SDL_MapGPUTransferBuffer(device, tb, false);
        for (int f = 0; f < faces; f++)
            memcpy(t->levels[f][0].data(), map + level * f, level);
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_ReleaseGPUTransferBuffer(device, tb);
    }

    // ---- pipelines and samplers ----

    // A draw's pipeline: everything of it the state can change.
    static uint32_t PipelineKey(const GfxCommand &cmd, const GfxDrawState &s)
    {
        bool depthTest = s.depthTest;
        return (cmd.transformed ? 1u : 0u) | (uint32_t)(cmd.primitive & 7) << 1 | (s.alphaBlend ? 1u : 0u) << 4 |
               (uint32_t)(s.srcBlend & 15) << 5 | (uint32_t)(s.destBlend & 15) << 9 | (depthTest ? 1u : 0u) << 13 |
               ((depthTest && s.depthWrite) ? 1u : 0u) << 14 | (uint32_t)(s.depthFunc & 15) << 15 |
               (uint32_t)(s.cull & 3) << 19;
    }

    SDL_GPUGraphicsPipeline *Pipeline(uint32_t key)
    {
        auto it = pipelines.find(key);
        if (it != pipelines.end())
            return it->second;
        bool transformed = (key & 1) != 0, alphaBlend = (key >> 4 & 1) != 0, depthTest = (key >> 13 & 1) != 0;
        bool depthWrite = (key >> 14 & 1) != 0;
        int primitive = key >> 1 & 7, srcBlend = key >> 5 & 15, destBlend = key >> 9 & 15, depthFunc = key >> 15 & 15;
        int cull = key >> 19 & 3;

        SDL_GPUVertexBufferDescription vb = {};
        vb.slot = 0;
        vb.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        SDL_GPUVertexAttribute attrs[6] = {};
        int attrCount;
        if (transformed) {
            vb.pitch = sizeof(GfxTLVertex);
            attrs[0] = { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0 };
            attrs[1] = { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 16 };
            attrs[2] = { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 20 };
            attrs[3] = { 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 24 };
            attrCount = 4;
        } else {
            vb.pitch = sizeof(GfxVertex);
            attrs[0] = { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0 };
            attrs[1] = { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 12 };
            attrs[2] = { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 24 };
            attrs[3] = { 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 28 };
            attrs[4] = { 4, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32 };
            attrs[5] = { 5, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 40 };
            attrCount = 6;
        }

        SDL_GPUColorTargetDescription color = {};
        color.format = colorFormat;
        color.blend_state.enable_blend = alphaBlend;
        color.blend_state.src_color_blendfactor = BlendFactor(srcBlend);
        color.blend_state.dst_color_blendfactor = BlendFactor(destBlend);
        color.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        color.blend_state.src_alpha_blendfactor = BlendFactor(srcBlend);
        color.blend_state.dst_alpha_blendfactor = BlendFactor(destBlend);
        color.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = transformed ? vsTL : vs3d;
        info.fragment_shader = fs;
        info.vertex_input_state.vertex_buffer_descriptions = &vb;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attrs;
        info.vertex_input_state.num_vertex_attributes = attrCount;
        info.primitive_type = PrimitiveType(primitive);
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        // Direct3D's front faces are clockwise; CULL_CCW (its default)
        // removes the back faces.
        info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
        info.rasterizer_state.cull_mode = cull == GFX_CULL_CW    ? SDL_GPU_CULLMODE_FRONT
                                          : cull == GFX_CULL_CCW ? SDL_GPU_CULLMODE_BACK
                                                                   : SDL_GPU_CULLMODE_NONE;
        info.rasterizer_state.enable_depth_clip = true;
        info.depth_stencil_state.enable_depth_test = depthTest;
        info.depth_stencil_state.enable_depth_write = depthTest && depthWrite;
        info.depth_stencil_state.compare_op = CompareOp(depthFunc);
        info.target_info.color_target_descriptions = &color;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (p == NULL)
            Sys_Log("gfx: pipeline: %s", SDL_GetError());
        pipelines[key] = p;
        return p;
    }

    SDL_GPUGraphicsPipeline *ClearPipeline(bool color, bool depth)
    {
        uint32_t key = 0x80000000u | (color ? 1u : 0u) | (depth ? 2u : 0u);
        auto it = pipelines.find(key);
        if (it != pipelines.end())
            return it->second;
        SDL_GPUColorTargetDescription target = {};
        target.format = colorFormat;
        target.blend_state.enable_color_write_mask = true;
        target.blend_state.color_write_mask = color ? 0xf : 0;
        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = vsClear;
        info.fragment_shader = fsClear;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.depth_stencil_state.enable_depth_test = depth;
        info.depth_stencil_state.enable_depth_write = depth;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = depthFormat;
        info.target_info.has_depth_stencil_target = true;
        SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(device, &info);
        if (p == NULL)
            Sys_Log("gfx: clear pipeline: %s", SDL_GetError());
        pipelines[key] = p;
        return p;
    }

    SDL_GPUSampler *Sampler(const GfxStageState &stage)
    {
        int aniso = anisotropy;
        uint32_t key = (uint32_t)(stage.minFilter & 3) | (uint32_t)(stage.magFilter & 3) << 2 |
                       (uint32_t)(stage.mipFilter & 3) << 4 | (uint32_t)(stage.address & 3) << 6 |
                       (uint32_t)(aniso & 31) << 8;
        auto it = samplers.find(key);
        if (it != samplers.end())
            return it->second;
        SDL_GPUSamplerCreateInfo info = {};
        info.min_filter = stage.minFilter == GFX_FILTER_LINEAR ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
        info.mag_filter = stage.magFilter == GFX_FILTER_LINEAR ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
        info.mipmap_mode = stage.mipFilter == GFX_MIPFILTER_LINEAR ? SDL_GPU_SAMPLERMIPMAPMODE_LINEAR
                                                                   : SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        SDL_GPUSamplerAddressMode mode = stage.address == GFX_ADDRESS_CLAMP    ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
                                         : stage.address == GFX_ADDRESS_MIRROR ? SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT
                                                                               : SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        info.address_mode_u = info.address_mode_v = info.address_mode_w = mode;
        info.min_lod = 0.0f;
        info.max_lod = stage.mipFilter == GFX_MIPFILTER_NONE ? 0.0f : 1000.0f;
        if (aniso > 1 && stage.mipFilter != GFX_MIPFILTER_NONE) {
            info.enable_anisotropy = true;
            info.max_anisotropy = (float)aniso;
        }
        SDL_GPUSampler *s = SDL_CreateGPUSampler(device, &info);
        samplers[key] = s;
        return s;
    }

    // ---- uniforms ----

    // Whether two states give the same VertexUniforms (FillVertexUniforms).
    static bool SameVertexUniforms(const GfxDrawState &a, const GfxDrawState &b)
    {
        for (int i = 0; i < 2; i++)
            if (a.stages[i].texCoordIndex != b.stages[i].texCoordIndex ||
                a.stages[i].textureTransform != b.stages[i].textureTransform)
                return false;
        return a.viewport.width == b.viewport.width && a.viewport.height == b.viewport.height &&
               a.screenW == b.screenW && a.screenH == b.screenH && a.lighting == b.lighting && a.colorVertex == b.colorVertex && a.specular == b.specular &&
               a.localViewer == b.localViewer && a.normalizeNormals == b.normalizeNormals &&
               a.diffuseSource == b.diffuseSource && a.specularSource == b.specularSource &&
               a.ambientSource == b.ambientSource && a.emissiveSource == b.emissiveSource && a.fog == b.fog &&
               a.fogTableMode == b.fogTableMode && a.fogVertexMode == b.fogVertexMode &&
               a.fogStart == b.fogStart && a.fogEnd == b.fogEnd && a.ambient == b.ambient &&
               memcmp(&a.world, &b.world, sizeof(a.world)) == 0 && memcmp(&a.view, &b.view, sizeof(a.view)) == 0 &&
               memcmp(&a.projection, &b.projection, sizeof(a.projection)) == 0 &&
               memcmp(a.texture, b.texture, sizeof(a.texture)) == 0 &&
               memcmp(&a.material, &b.material, sizeof(a.material)) == 0 &&
               memcmp(a.lightEnabled, b.lightEnabled, sizeof(a.lightEnabled)) == 0 &&
               memcmp(a.lights, b.lights, sizeof(a.lights)) == 0;
    }

    // A rectangle of the screen the game draws in (Gfx_SetVirtualScreen) in
    // the back buffer's pixels, within it; elsewhere as it is.
    static GfxViewport ToTarget(const GfxViewport &v, int screenW, int screenH, bool backBuffer, int targetW,
                                int targetH)
    {
        if (!backBuffer || screenW <= 0 || screenH <= 0)
            return v;
        float kx = (float)targetW / screenW, ky = (float)targetH / screenH;
        int x0 = std::clamp((int)lroundf(v.x * kx), 0, targetW);
        int y0 = std::clamp((int)lroundf(v.y * ky), 0, targetH);
        int x1 = std::clamp((int)lroundf((v.x + v.width) * kx), x0, targetW);
        int y1 = std::clamp((int)lroundf((v.y + v.height) * ky), y0, targetH);
        GfxViewport r = v;
        r.x = x0;
        r.y = y0;
        r.width = x1 - x0;
        r.height = y1 - y0;
        return r;
    }

    void FillVertexUniforms(const GfxCommand &cmd, const GfxDrawState &s, int targetW, int targetH,
                            const GfxViewport &viewport, bool scaled, VertexUniforms &u)
    {
        memset(&u, 0, sizeof(u));
        Multiply(s.world, s.view, u.worldView);
        memcpy(u.projection, &s.projection, sizeof(u.projection));
        memcpy(u.textureMatrix[0], &s.texture[0], 64);
        memcpy(u.textureMatrix[1], &s.texture[1], 64);
        Copy4(s.material.diffuse, u.materialDiffuse);
        Copy4(s.material.ambient, u.materialAmbient);
        Copy4(s.material.specular, u.materialSpecular);
        Copy4(s.material.emissive, u.materialEmissive);
        Color(s.ambient, u.globalAmbient);
        u.globalAmbient[3] = s.material.power;
        for (int i = 0; i < 8; i++) {
            const GfxLight &l = s.lights[i];
            LightUniform &lu = u.lights[i];
            Copy4(l.diffuse, lu.diffuse);
            Copy4(l.specular, lu.specular);
            Copy4(l.ambient, lu.ambient);
            TransformPoint(l.position, 1.0f, s.view, lu.position);
            lu.position[3] = (float)l.type;
            TransformPoint(l.direction, 0.0f, s.view, lu.direction);
            lu.direction[3] = l.range > 0.0f ? l.range : 1e30f;
            lu.attenuation[0] = l.attenuation0;
            lu.attenuation[1] = l.attenuation1;
            lu.attenuation[2] = l.attenuation2;
            lu.attenuation[3] = l.falloff;
            lu.spot[0] = cosf(l.theta * 0.5f);
            lu.spot[1] = cosf(l.phi * 0.5f);
            lu.spot[2] = s.lightEnabled[i] ? 1.0f : 0.0f;
        }
        u.lightFlags[0] = s.lighting;
        u.lightFlags[1] = s.colorVertex;
        u.lightFlags[2] = s.specular;
        u.lightFlags[3] = s.localViewer;
        u.sources[0] = s.diffuseSource;
        u.sources[1] = s.specularSource;
        u.sources[2] = s.ambientSource;
        u.sources[3] = s.emissiveSource;
        u.fogFlags[0] = (s.fog && s.fogTableMode == GFX_FOG_NONE) ? s.fogVertexMode : 0;
        u.fogFlags[1] = s.normalizeNormals;
        // Direct3D takes the fog factor from the vertex's specular alpha when
        // fog is on with neither vertex nor table fog: the backdrop of a
        // foggy stage is drawn so, its vertices' alpha 0 filling it with fog.
        u.fogFlags[2] = s.fog && s.fogTableMode == GFX_FOG_NONE && s.fogVertexMode == GFX_FOG_NONE;
        u.fogParams[0] = s.fogStart;
        u.fogParams[1] = s.fogEnd;
        u.texCoordIndex[0] = (int32_t)s.stages[0].texCoordIndex;
        u.texCoordIndex[1] = (int32_t)s.stages[1].texCoordIndex;
        u.texCoordIndex[2] = s.stages[0].textureTransform;
        u.texCoordIndex[3] = s.stages[1].textureTransform;
        // Pre-transformed vertices are in pixels of the game's screen, scaled
        // to the back buffer's.
        u.target[0] = (float)(scaled ? s.screenW : targetW);
        u.target[1] = (float)(scaled ? s.screenH : targetH);
        // Direct3D 7's half-pixel offset of the viewport transform.
        u.target[2] = viewport.width ? 1.0f / viewport.width : 0.0f;
        u.target[3] = viewport.height ? -1.0f / viewport.height : 0.0f;
        (void)cmd;
    }

    int TextureKind(const GfxStageState &stage)
    {
        GfxTexture *t = Gfx_DrawnTexture(stage.texture);
        if (t == NULL)
            return 0;
        if (t->flags & GFX_TEXTURE_CUBE)
            return 2;
        if (t->format == GFX_FORMAT_BUMP_DUDVL)
            return 3;
        if (t->flags & GFX_TEXTURE_DISTANCE_FIELD)
            return 4;
        return 1;
    }

    // A 2D texture drawn with point filtering and no mipmaps (the game's
    // sprites, text and HUD). Scaled, nearest texels would come out one or
    // two pixels wide, or be skipped; the shader filters them smoothly
    // through a linear sampler. At their size it reads them as the point
    // filter does.
    bool ScaledPoint(const GfxStageState &stage)
    {
        return TextureKind(stage) == 1 && stage.magFilter == GFX_FILTER_POINT && stage.mipFilter == GFX_MIPFILTER_NONE;
    }

    // fade: the draw is a 3D one into the back buffer, which the distance
    // fade applies to.
    void FillFragmentUniforms(const GfxDrawState &s, bool fade, FragmentUniforms &u)
    {
        memset(&u, 0, sizeof(u));
        const GfxStageState &a = s.stages[0], &b = s.stages[1];
        u.stage0[0] = a.colorOp;
        u.stage0[1] = a.colorArg1;
        u.stage0[2] = a.colorArg2;
        u.stage0[3] = a.alphaOp;
        u.stage0b[0] = a.alphaArg1;
        u.stage0b[1] = a.alphaArg2;
        u.stage0b[2] = TextureKind(a);
        u.stage0b[3] = ScaledPoint(a);
        u.stage1[0] = b.colorOp;
        u.stage1[1] = b.colorArg1;
        u.stage1[2] = b.colorArg2;
        u.stage1[3] = b.alphaOp;
        u.stage1b[0] = b.alphaArg1;
        u.stage1b[1] = b.alphaArg2;
        u.stage1b[2] = TextureKind(b);
        u.stage1b[3] = ScaledPoint(b);
        Color(s.textureFactor, u.textureFactor);
        u.alphaTest[0] = s.alphaTest;
        u.alphaTest[1] = s.alphaFunc;
        u.alphaRef[0] = (s.alphaRef & 0xff) / 255.0f;
        Color(s.fogColor, u.fogColor);
        u.fogMode[0] = s.fog;
        u.fogMode[1] = s.fogTableMode;
        u.fogMode[2] = s.specular;
        u.fogParams[0] = s.fogStart;
        u.fogParams[1] = s.fogEnd;
        memcpy(u.bumpMatrix, a.bumpMat, sizeof(u.bumpMatrix));
        u.bumpLuminance[0] = a.bumpLumScale;
        u.bumpLuminance[1] = a.bumpLumOffset;
        if (fade && s.fadeEnd > s.fadeStart) {
            u.distanceFade[0] = s.fadeStart;
            u.distanceFade[1] = s.fadeEnd;
            u.distanceFade[2] = 1.0f;
        }
    }

    // The stages' textures (2D and cube for each), then the backdrop.
    void TextureBindings(const GfxDrawState &s, SDL_GPUTextureSamplerBinding bindings[5])
    {
        for (int i = 0; i < 2; i++) {
            const GfxStageState &stage = s.stages[i];
            GfxTexture *t = Gfx_DrawnTexture(stage.texture);
            GpuTexture *g = t != NULL ? Ensure(t) : NULL;
            GfxStageState sampled = stage;
            if (ScaledPoint(stage))
                sampled.minFilter = sampled.magFilter = GFX_FILTER_LINEAR;
            // A distance field is read between its texels, and from its mip
            // levels where it is drawn smaller.
            if (TextureKind(stage) == 4) {
                sampled.minFilter = sampled.magFilter = GFX_FILTER_LINEAR;
                sampled.mipFilter = GFX_MIPFILTER_LINEAR;
            }
            SDL_GPUSampler *sampler = Sampler(sampled);
            bool cube = t != NULL && (t->flags & GFX_TEXTURE_CUBE);
            SDL_GPUTexture *texture = g != NULL ? g->texture : NULL;
            bindings[i * 2] = { (texture != NULL && !cube) ? texture : white2D, sampler };
            bindings[i * 2 + 1] = { (texture != NULL && cube) ? texture : whiteCube, sampler };
        }
        bindings[4] = { backdrop, Sampler(GfxStageState()) };
    }

    // What a render pass has bound: each draw sets only what differs from the
    // previous one. Render passes start with nothing bound.
    struct PassBindings {
        SDL_GPUGraphicsPipeline *pipeline = nullptr;
        bool viewportSet = false, texturesSet = false, indicesSet = false;
        SDL_GPUViewport viewport = {};
        SDL_Rect scissor = {};
        SDL_GPUTextureSamplerBinding textures[5] = {};
        SDL_GPUBuffer *vertices = nullptr;
    };

    void SetViewport(SDL_GPURenderPass *pass, PassBindings &bound, const SDL_GPUViewport &vp, const SDL_Rect &scissor)
    {
        if (bound.viewportSet && memcmp(&bound.viewport, &vp, sizeof(vp)) == 0 &&
            memcmp(&bound.scissor, &scissor, sizeof(scissor)) == 0)
            return;
        SDL_SetGPUViewport(pass, &vp);
        SDL_SetGPUScissor(pass, &scissor);
        bound.viewport = vp;
        bound.scissor = scissor;
        bound.viewportSet = true;
    }

    // ---- frame execution ----

    void EnsureBuffers(uint32_t vertexBytes, uint32_t indexBytes)
    {
        if (vertexBytes > vertexCapacity) {
            SDL_ReleaseGPUBuffer(device, vertexBuffer);
            vertexCapacity = std::max(vertexBytes, vertexCapacity * 2);
            SDL_GPUBufferCreateInfo info = { SDL_GPU_BUFFERUSAGE_VERTEX, vertexCapacity, 0 };
            vertexBuffer = SDL_CreateGPUBuffer(device, &info);
        }
        if (indexBytes > indexCapacity) {
            SDL_ReleaseGPUBuffer(device, indexBuffer);
            indexCapacity = std::max(indexBytes, indexCapacity * 2);
            SDL_GPUBufferCreateInfo info = { SDL_GPU_BUFFERUSAGE_INDEX, indexCapacity, 0 };
            indexBuffer = SDL_CreateGPUBuffer(device, &info);
        }
    }

    void UploadGeometry(SDL_GPUCommandBuffer *cmd, GfxFrame &frame)
    {
        uint32_t vertexBytes = (uint32_t)frame.vertices.size();
        uint32_t indexBytes = (uint32_t)(frame.indices.size() * 2);
        if (vertexBytes == 0 && indexBytes == 0)
            return;
        EnsureBuffers(std::max(vertexBytes, 4u), std::max(indexBytes, 4u));
        if (vertexBytes + indexBytes > uploadCapacity) {
            SDL_ReleaseGPUTransferBuffer(device, uploadBuffer);
            uploadCapacity = std::max(vertexBytes + indexBytes, uploadCapacity * 2);
            SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, uploadCapacity, 0 };
            uploadBuffer = SDL_CreateGPUTransferBuffer(device, &tbi);
        }
        SDL_GPUTransferBuffer *tb = uploadBuffer;
        // Cycled: a fresh area while the GPU still reads an earlier frame's.
        uint8_t *map = (uint8_t *)SDL_MapGPUTransferBuffer(device, tb, true);
        memcpy(map, frame.vertices.data(), vertexBytes);
        if (indexBytes)
            memcpy(map + vertexBytes, frame.indices.data(), indexBytes);
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        if (vertexBytes) {
            SDL_GPUTransferBufferLocation src = { tb, 0 };
            SDL_GPUBufferRegion dst = { vertexBuffer, 0, vertexBytes };
            SDL_UploadToGPUBuffer(copy, &src, &dst, true);
        }
        if (indexBytes) {
            SDL_GPUTransferBufferLocation isrc = { tb, vertexBytes };
            SDL_GPUBufferRegion idst = { indexBuffer, 0, indexBytes };
            SDL_UploadToGPUBuffer(copy, &isrc, &idst, true);
        }
        SDL_EndGPUCopyPass(copy);
    }

    // The GPU copies of the vertex buffers the frame draws from, where the
    // game has locked them since.
    void UploadVertexBuffers(SDL_GPUCommandBuffer *cmd, GfxFrame &frame)
    {
        SDL_GPUCopyPass *copy = NULL;
        for (GfxVertexBuffer *b : frame.buffers) {
            GpuVertexBuffer *g = (GpuVertexBuffer *)b->gpu;
            if (g == NULL)
                b->gpu = g = new GpuVertexBuffer;
            uint32_t bytes = (uint32_t)(b->vertices.size() * sizeof(GfxVertex));
            if ((g->version == b->version && g->buffer != NULL) || bytes == 0)
                continue;
            if (bytes > g->capacity) {
                SDL_ReleaseGPUBuffer(device, g->buffer);
                SDL_GPUBufferCreateInfo info = { SDL_GPU_BUFFERUSAGE_VERTEX, bytes, 0 };
                g->buffer = SDL_CreateGPUBuffer(device, &info);
                g->capacity = bytes;
            }
            SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, bytes, 0 };
            SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
            memcpy(SDL_MapGPUTransferBuffer(device, tb, false), b->vertices.data(), bytes);
            SDL_UnmapGPUTransferBuffer(device, tb);
            if (copy == NULL)
                copy = SDL_BeginGPUCopyPass(cmd);
            SDL_GPUTransferBufferLocation src = { tb, 0 };
            SDL_GPUBufferRegion dst = { g->buffer, 0, bytes };
            // Cycling keeps the contents earlier frames still draw from.
            SDL_UploadToGPUBuffer(copy, &src, &dst, true);
            SDL_ReleaseGPUTransferBuffer(device, tb);
            g->version = b->version;
        }
        if (copy != NULL)
            SDL_EndGPUCopyPass(copy);
    }

    void DestroyVertexBuffer(GfxVertexBuffer *b) override
    {
        GpuVertexBuffer *g = (GpuVertexBuffer *)b->gpu;
        if (g == NULL)
            return;
        if (device != NULL)
            SDL_ReleaseGPUBuffer(device, g->buffer);
        delete g;
        b->gpu = NULL;
    }

    // Begins a pass on the target at commands[first]. Clears of the whole
    // target there, before any draw, become the pass's load operations (the
    // GPU's fast clears) instead of drawn quads; *cleared counts them.
    SDL_GPURenderPass *BeginPass(SDL_GPUCommandBuffer *cmd, GfxTexture *target, int &w, int &h,
                                 const std::vector<GfxCommand> &commands, size_t first, size_t *cleared)
    {
        SDL_GPUColorTargetInfo color = {};
        SDL_GPUDepthStencilTargetInfo depth = {};
        color.load_op = SDL_GPU_LOADOP_LOAD;
        color.store_op = SDL_GPU_STOREOP_STORE;
        depth.load_op = SDL_GPU_LOADOP_LOAD;
        depth.store_op = SDL_GPU_STOREOP_STORE;
        depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
        if (target == NULL) {
            color.texture = backColor;
            depth.texture = backDepth;
            w = backWidth;
            h = backHeight;
        } else {
            GfxTexture *root = target->cube != NULL ? target->cube : target;
            GpuTexture *g = Ensure(root);
            color.texture = g->texture;
            color.layer_or_depth_plane = target->cube != NULL ? target->face : 0;
            depth.texture = g->depth;
            w = root->width;
            h = root->height;
        }
        *cleared = 0;
        for (size_t i = first; i < commands.size() && commands[i].type == GFX_CMD_CLEAR; i++) {
            const GfxCommand &c = commands[i];
            GfxViewport v = ToTarget(c.clearViewport, c.clearScreenW, c.clearScreenH, target == NULL, w, h);
            if (v.x != 0 || v.y != 0 || v.width != (DWORD)w || v.height != (DWORD)h)
                break;
            if (c.clearFlags & GFX_CLEAR_TARGET) {
                float rgba[4];
                Color(c.clearColor, rgba);
                color.load_op = SDL_GPU_LOADOP_CLEAR;
                color.clear_color = { rgba[0], rgba[1], rgba[2], rgba[3] };
            }
            if (c.clearFlags & GFX_CLEAR_ZBUFFER) {
                depth.load_op = SDL_GPU_LOADOP_CLEAR;
                depth.clear_depth = c.clearZ;
            }
            (*cleared)++;
        }
        return SDL_BeginGPURenderPass(cmd, &color, 1, &depth);
    }

    void Execute(GfxFrame &frame, std::vector<GfxTexture *> &textures, GfxReadback &readback, bool present,
                 bool vsync, bool stretch) override
    {
        if (device == NULL)
            return;
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        if (cmd == NULL) {
            Sys_Log("gfx: %s", SDL_GetError());
            return;
        }
        // Textures used by this frame must exist before the passes start.
        for (const GfxDrawState &s : frame.states)
            for (const GfxStageState &stage : s.stages)
                if (stage.texture != NULL)
                    Ensure(Gfx_DrawnTexture(stage.texture));
        for (const GfxCommand &c : frame.commands)
            if (c.type == GFX_CMD_TARGET && c.target != NULL)
                Ensure(c.target->cube != NULL ? c.target->cube : c.target);
        UploadTextures(cmd, textures);
        UploadGeometry(cmd, frame);
        UploadVertexBuffers(cmd, frame);

        SDL_GPURenderPass *pass = NULL;
        PassBindings bound;
        GfxTexture *target = NULL;
        int targetW = backWidth, targetH = backHeight;
        // The uniforms pushed to this command buffer (they stay for its
        // passes): the state and target size of the vertex ones, and the
        // fragment ones. Clears push their own.
        const GfxDrawState *vertexUniformState = NULL;
        int vertexUniformW = 0, vertexUniformH = 0;
        bool vertexUniformScaled = false;
        FragmentUniforms fragmentUniforms;
        bool fragmentUniformsSet = false;
        for (size_t i = 0; i < frame.commands.size(); i++) {
            const GfxCommand &c = frame.commands[i];
            if (c.type == GFX_CMD_TARGET) {
                if (pass != NULL) {
                    SDL_EndGPURenderPass(pass);
                    pass = NULL;
                }
                target = c.target;
                continue;
            }
            if (c.type == GFX_CMD_BACKDROP) {
                // A copy pass between render passes; the next draw starts a
                // render pass that loads the back buffer as it is.
                if (target != NULL)
                    continue;
                if (pass != NULL) {
                    SDL_EndGPURenderPass(pass);
                    pass = NULL;
                }
                SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
                SDL_GPUTextureLocation src = {}, dst = {};
                src.texture = backColor;
                dst.texture = backdrop;
                SDL_CopyGPUTextureToTexture(copy, &src, &dst, backWidth, backHeight, 1, false);
                SDL_EndGPUCopyPass(copy);
                continue;
            }
            if (pass == NULL) {
                size_t cleared;
                pass = BeginPass(cmd, target, targetW, targetH, frame.commands, i, &cleared);
                bound = PassBindings();
                if (cleared > 0) {
                    i += cleared - 1;
                    continue;
                }
            }
            if (c.type == GFX_CMD_CLEAR) {
                bool colorClear = (c.clearFlags & GFX_CLEAR_TARGET) != 0;
                bool depthClear = (c.clearFlags & GFX_CLEAR_ZBUFFER) != 0;
                if (!colorClear && !depthClear)
                    continue;
                SDL_GPUGraphicsPipeline *pipeline = ClearPipeline(colorClear, depthClear);
                if (pipeline != bound.pipeline) {
                    SDL_BindGPUGraphicsPipeline(pass, pipeline);
                    bound.pipeline = pipeline;
                }
                GfxViewport v = ToTarget(c.clearViewport, c.clearScreenW, c.clearScreenH, target == NULL, targetW,
                                         targetH);
                SDL_GPUViewport vp = { (float)v.x, (float)v.y, (float)v.width, (float)v.height, 0.0f, 1.0f };
                SDL_Rect scissor = { (int)v.x, (int)v.y, (int)v.width, (int)v.height };
                SetViewport(pass, bound, vp, scissor);
                ClearUniforms u = {};
                Color(c.clearColor, u.color);
                u.depth[0] = c.clearZ;
                SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));
                SDL_PushGPUFragmentUniformData(cmd, 0, &u, sizeof(u));
                vertexUniformState = NULL;
                fragmentUniformsSet = false;
                SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
                continue;
            }
            const GfxDrawState &s = frame.states[c.state];
            SDL_GPUGraphicsPipeline *pipeline = Pipeline(PipelineKey(c, s));
            if (pipeline == NULL)
                continue;
            if (pipeline != bound.pipeline) {
                SDL_BindGPUGraphicsPipeline(pass, pipeline);
                bound.pipeline = pipeline;
            }
            const bool scaled = target == NULL && s.screenW > 0 && s.screenH > 0;
            const GfxViewport viewport = ToTarget(s.viewport, s.screenW, s.screenH, target == NULL, targetW, targetH);
            SDL_GPUViewport vp;
            if (c.transformed)
                vp = { 0.0f, 0.0f, (float)targetW, (float)targetH, 0.0f, 1.0f };
            else
                vp = { (float)viewport.x, (float)viewport.y, (float)viewport.width, (float)viewport.height,
                       viewport.minZ, viewport.maxZ };
            SDL_Rect scissor = { (int)viewport.x, (int)viewport.y, (int)viewport.width, (int)viewport.height };
            SetViewport(pass, bound, vp, scissor);
            if (vertexUniformState == NULL || vertexUniformW != targetW || vertexUniformH != targetH ||
                vertexUniformScaled != scaled ||
                (vertexUniformState != &s && !SameVertexUniforms(*vertexUniformState, s))) {
                VertexUniforms vu;
                FillVertexUniforms(c, s, targetW, targetH, viewport, scaled, vu);
                SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
                vertexUniformState = &s;
                vertexUniformW = targetW;
                vertexUniformH = targetH;
                vertexUniformScaled = scaled;
            }
            FragmentUniforms fu;
            FillFragmentUniforms(s, target == NULL && !c.transformed, fu);
            if (!fragmentUniformsSet || memcmp(&fu, &fragmentUniforms, sizeof(fu)) != 0) {
                SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
                fragmentUniforms = fu;
                fragmentUniformsSet = true;
            }
            SDL_GPUTextureSamplerBinding textures[5];
            TextureBindings(s, textures);
            if (!bound.texturesSet || memcmp(textures, bound.textures, sizeof(textures)) != 0) {
                SDL_BindGPUFragmentSamplers(pass, 0, textures, 5);
                memcpy(bound.textures, textures, sizeof(textures));
                bound.texturesSet = true;
            }
            // The buffer from its start; the draw gives its first vertex.
            SDL_GPUBuffer *vertices = c.buffer != NULL ? ((GpuVertexBuffer *)c.buffer->gpu)->buffer : vertexBuffer;
            uint32_t firstVertex = c.firstVertexByte / (c.transformed ? sizeof(GfxTLVertex) : sizeof(GfxVertex));
            if (vertices != bound.vertices) {
                SDL_GPUBufferBinding vb = { vertices, 0 };
                SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
                bound.vertices = vertices;
            }
            if (c.firstIndex != ~0u) {
                if (!bound.indicesSet) {
                    SDL_GPUBufferBinding ib = { indexBuffer, 0 };
                    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
                    bound.indicesSet = true;
                }
                SDL_DrawGPUIndexedPrimitives(pass, c.indexCount, 1, c.firstIndex, (Sint32)firstVertex, 0);
            } else {
                SDL_DrawGPUPrimitives(pass, c.vertexCount, 1, firstVertex, 0);
            }
        }
        if (pass != NULL)
            SDL_EndGPURenderPass(pass);

        if (!present) {
            SDL_SubmitGPUCommandBuffer(cmd);
            return;
        }

        // Read back before the frame is shown (the lens flare test).
        CollectReadback(readback);
        bool reading = false;
        if (readback.requested && readback.width > 0 && readback.height > 0) {
            int x = std::max(0, std::min(readback.x, backWidth - 1));
            int y = std::max(0, std::min(readback.y, backHeight - 1));
            int w = std::min(readback.width, backWidth - x), h = std::min(readback.height, backHeight - y);
            if ((uint32_t)(w * h * 4) > readbackCapacity) {
                SDL_ReleaseGPUTransferBuffer(device, readbackBuffer);
                readbackCapacity = (uint32_t)(w * h * 4);
                SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, readbackCapacity, 0 };
                readbackBuffer = SDL_CreateGPUTransferBuffer(device, &tbi);
            }
            SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
            SDL_GPUTextureRegion src = {};
            src.texture = backColor;
            src.x = x;
            src.y = y;
            src.w = w;
            src.h = h;
            src.d = 1;
            SDL_GPUTextureTransferInfo dst = { readbackBuffer, 0, 0, 0 };
            SDL_DownloadFromGPUTexture(copy, &src, &dst);
            SDL_EndGPUCopyPass(copy);
            readbackWidth = w;
            readbackHeight = h;
            readback.requested = false;
            reading = true;
        }

        if (vsync != presentModeVsync) {
            SDL_GPUPresentMode mode = vsync ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE;
            if (!vsync && !SDL_WindowSupportsGPUPresentMode(device, window, mode))
                mode = SDL_GPU_PRESENTMODE_MAILBOX;
            if (SDL_WindowSupportsGPUPresentMode(device, window, mode))
                SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode);
            presentModeVsync = vsync;
        }

        SDL_GPUTexture *swapchain = NULL;
        Uint32 sw = 0, sh = 0;
        if (SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain, &sw, &sh) && swapchain != NULL) {
            if (sw != presentWidth || sh != presentHeight) {
                Sys_Log("gfx: %dx%d shown in %ux%u pixels", backWidth, backHeight, sw, sh);
                presentWidth = sw;
                presentHeight = sh;
            }
            // Movie frames fill the drawable; ordinary game frames retain their aspect ratio.
            PresentationRect rect = GetPresentationRect(backWidth, backHeight, sw, sh, stretch);
            SDL_GPUBlitInfo blit = {};
            blit.source.texture = backColor;
            blit.source.w = backWidth;
            blit.source.h = backHeight;
            blit.destination.texture = swapchain;
            blit.destination.x = rect.x;
            blit.destination.y = rect.y;
            blit.destination.w = rect.width;
            blit.destination.h = rect.height;
            blit.load_op = SDL_GPU_LOADOP_CLEAR;
            blit.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
            blit.filter = smoothScaling ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
            SDL_BlitGPUTexture(cmd, &blit);
        }

        if (reading)
            readbackFence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        else
            SDL_SubmitGPUCommandBuffer(cmd);
        HashFrame();
    }

    void CollectReadback(GfxReadback &readback) override
    {
        if (readbackFence == NULL)
            return;
        SDL_WaitForGPUFences(device, true, &readbackFence, 1);
        SDL_ReleaseGPUFence(device, readbackFence);
        readbackFence = NULL;
        const uint32_t *map = (const uint32_t *)SDL_MapGPUTransferBuffer(device, readbackBuffer, false);
        readback.pixels.assign(map, map + (size_t)readbackWidth * readbackHeight);
        SDL_UnmapGPUTransferBuffer(device, readbackBuffer);
        readback.readyWidth = readbackWidth;
        readback.readyHeight = readbackHeight;
        readback.ready = true;
    }

    // The back buffer's pixels for Gfx_HashFrame.
    void HashFrame()
    {
        if (!Gfx_FrameHashWanted())
            return;
        uint32_t bytes = (uint32_t)backWidth * backHeight * 4;
        SDL_GPUTransferBufferCreateInfo tbi = { SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, bytes, 0 };
        SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(device, &tbi);
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTextureRegion src = {};
        src.texture = backColor;
        src.w = backWidth;
        src.h = backHeight;
        src.d = 1;
        SDL_GPUTextureTransferInfo dst = { tb, 0, 0, 0 };
        SDL_DownloadFromGPUTexture(copy, &src, &dst);
        SDL_EndGPUCopyPass(copy);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        SDL_WaitForGPUFences(device, true, &fence, 1);
        SDL_ReleaseGPUFence(device, fence);
        Gfx_HashFrame((const uint8_t *)SDL_MapGPUTransferBuffer(device, tb, false), backWidth, backHeight);
        SDL_UnmapGPUTransferBuffer(device, tb);
        SDL_ReleaseGPUTransferBuffer(device, tb);
    }
};

} // namespace

GfxExecutor *Gfx_CreateGpuExecutor(void)
{
    return new Executor;
}

bool Gfx_GpuIsAvailable(void)
{
    SDL_PropertiesID props = DeviceProperties();
    // A software Vulkan driver (lavapipe, often installed beside the GPU's
    // own driver) does not count: an old GPU draws faster on its OpenGL.
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_REQUIRE_HARDWARE_ACCELERATION_BOOLEAN, true);
    bool available = SDL_GPUSupportsProperties(props);
    SDL_DestroyProperties(props);
    return available;
}
