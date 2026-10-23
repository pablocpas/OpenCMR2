// The OpenGL 3.3 executor: runs a recorded frame (gfx_core.cpp) as the
// SDL_GPU one does (gfx_gpu.cpp), with its shaders translated to GLSL 330
// (src/CMakeLists.txt), on GPUs without Vulkan or Direct3D 12.
//
// Images keep Direct3D's rows: the shaders draw upside down, so the first
// row of every target is its top, as in the textures the game uploads and
// the back buffer it reads. Viewports and scissors take the game's top-based
// rows as they are, faces wind the other way round, and only the copy to the
// window turns the picture over.

#include "platform/platform.h"
#include "port/sys.h"
#include "render/presentation.h"
#include "render/gfx_internal.h"

#include "shaders_spirv.h"

// Declarations only: every function is loaded through SDL (Gl::Load), and
// these give the types of its pointers.
#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL_opengl.h>

#include <math.h>
#include <string.h>

#include <algorithm>
#include <string>
#include <unordered_map>

namespace {

// ---- functions ----

#define GL_FUNCTIONS(X)                                                                                         \
    X(GetString) X(GetStringi) X(GetIntegerv) X(GetFloatv) X(GetError) X(Enable) X(Disable) X(Viewport) X(Scissor)          \
    X(DepthRange) X(ClearColor) X(ClearDepth) X(Clear) X(ColorMask) X(DepthMask) X(DepthFunc) X(BlendFunc)    \
    X(CullFace) X(FrontFace) X(PixelStorei) X(ReadPixels) X(Finish)                                           \
    X(GenTextures) X(DeleteTextures) X(BindTexture) X(ActiveTexture) X(TexImage2D) X(TexSubImage2D)           \
    X(CompressedTexImage2D) X(TexParameteri) X(GetTexImage) X(CopyTexSubImage2D)                              \
    X(GenSamplers) X(DeleteSamplers) X(BindSampler) X(SamplerParameteri) X(SamplerParameterf)                 \
    X(GenBuffers) X(DeleteBuffers) X(BindBuffer) X(BufferData) X(BindBufferRange) X(MapBufferRange)           \
    X(UnmapBuffer) X(GenVertexArrays) X(DeleteVertexArrays) X(BindVertexArray) X(EnableVertexAttribArray)      \
    X(VertexAttribPointer) X(DrawArrays) X(DrawElementsBaseVertex)                                             \
    X(GenFramebuffers) X(DeleteFramebuffers) X(BindFramebuffer) X(FramebufferTexture2D)                        \
    X(FramebufferRenderbuffer) X(CheckFramebufferStatus) X(BlitFramebuffer) X(GenRenderbuffers)                \
    X(DeleteRenderbuffers) X(BindRenderbuffer) X(RenderbufferStorage)                                          \
    X(CreateShader) X(ShaderSource) X(CompileShader) X(GetShaderiv) X(GetShaderInfoLog) X(DeleteShader)        \
    X(CreateProgram) X(AttachShader) X(LinkProgram) X(GetProgramiv) X(GetProgramInfoLog) X(DeleteProgram)      \
    X(UseProgram) X(GetUniformBlockIndex) X(UniformBlockBinding) X(GetUniformLocation) X(Uniform1i)            \
    X(FenceSync) X(ClientWaitSync) X(DeleteSync)

struct Gl {
#define GL_FUNCTION_POINTER(name) decltype(&::gl##name) name = nullptr;
    GL_FUNCTIONS(GL_FUNCTION_POINTER)
#undef GL_FUNCTION_POINTER

    bool Load()
    {
        bool ok = true;
#define LOAD_GL_FUNCTION(name)                                                                                           \
    name = (decltype(name))SDL_GL_GetProcAddress("gl" #name);                                                   \
    if (name == nullptr) {                                                                                      \
        Sys_Log("gfx: OpenGL has no gl" #name);                                                                 \
        ok = false;                                                                                             \
    }
        GL_FUNCTIONS(LOAD_GL_FUNCTION)
#undef LOAD_GL_FUNCTION
        return ok;
    }
};

Gl gl;

// ---- uniform blocks (std140, as gfx_gpu.cpp's) ----

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

enum { VERTEX_BLOCK, FRAGMENT_BLOCK };

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

GLenum BlendFactor(int d3d)
{
    switch (d3d) {
    case GFX_BLEND_ZERO: return GL_ZERO;
    case GFX_BLEND_ONE: return GL_ONE;
    case GFX_BLEND_SRCCOLOR: return GL_SRC_COLOR;
    case GFX_BLEND_INVSRCCOLOR: return GL_ONE_MINUS_SRC_COLOR;
    case GFX_BLEND_SRCALPHA: return GL_SRC_ALPHA;
    case GFX_BLEND_INVSRCALPHA: return GL_ONE_MINUS_SRC_ALPHA;
    case GFX_BLEND_DESTALPHA: return GL_DST_ALPHA;
    case GFX_BLEND_INVDESTALPHA: return GL_ONE_MINUS_DST_ALPHA;
    case GFX_BLEND_DESTCOLOR: return GL_DST_COLOR;
    case GFX_BLEND_INVDESTCOLOR: return GL_ONE_MINUS_DST_COLOR;
    case GFX_BLEND_SRCALPHASAT: return GL_SRC_ALPHA_SATURATE;
    default: return GL_ONE;
    }
}

GLenum CompareFunc(int d3d)
{
    switch (d3d) {
    case GFX_CMP_NEVER: return GL_NEVER;
    case GFX_CMP_LESS: return GL_LESS;
    case GFX_CMP_EQUAL: return GL_EQUAL;
    case GFX_CMP_LESSEQUAL: return GL_LEQUAL;
    case GFX_CMP_GREATER: return GL_GREATER;
    case GFX_CMP_NOTEQUAL: return GL_NOTEQUAL;
    case GFX_CMP_GREATEREQUAL: return GL_GEQUAL;
    default: return GL_ALWAYS;
    }
}

GLenum PrimitiveMode(int d3d)
{
    switch (d3d) {
    case GFX_POINTLIST: return GL_POINTS;
    case GFX_LINELIST: return GL_LINES;
    case GFX_LINESTRIP: return GL_LINE_STRIP;
    case GFX_TRIANGLESTRIP: return GL_TRIANGLE_STRIP;
    default: return GL_TRIANGLES;
    }
}

struct GlTexture {
    GLuint texture = 0;
    GLuint depth = 0;               // render targets: depth and stencil
    GLuint framebuffers[6] = {};    // render targets: one per face
    bool compressed = false;        // BC data uploaded as it is
    bool specified = false;         // storage made (later uploads replace pixels)
};

struct GlVertexBuffer {
    GLuint buffer = 0;
    uint32_t version = 0;
};

// What a draw needs besides its state: the uniform blocks written for it.
struct DrawUniforms {
    GLintptr vertexOffset = -1, fragmentOffset = -1;
};

struct Executor : GfxExecutor {
    SDL_Window *window = nullptr;
    SDL_GLContext context = nullptr;
    GLuint program3d = 0, programTL = 0;
    GLuint vertexArray = 0, vertexBuffer = 0, indexBuffer = 0, uniformBuffer = 0;
    GLuint backFramebuffer = 0, backColor = 0, backDepth = 0;
    GLuint backdrop = 0;    // the copy of backColor the distance fade reads
    int backWidth = 0, backHeight = 0;
    GLuint white2D = 0, whiteCube = 0;
    GLint uniformAlignment = 256;
    std::unordered_map<uint32_t, GLuint> samplers;
    bool bcSupported = false;
    float maxAnisotropy = 0.0f;     // 0: no anisotropic filtering
    int anisotropy = 1, smoothScaling = 1;
    int swapInterval = -2;          // as last set
    int presentWidth = 0, presentHeight = 0;
    // The back buffer read of the last presented frame (the lens flare test),
    // into a pixel buffer, collected when the game reads it.
    GLuint readbackBuffer = 0;
    GLsync readbackFence = nullptr;
    int readbackWidth = 0, readbackHeight = 0;
    std::vector<uint8_t> uniformData;
    std::vector<DrawUniforms> drawUniforms;

    // The state set in OpenGL, so that draws change only what differs.
    struct Bound {
        GLuint program = 0, framebuffer = ~0u, vertices = ~0u;
        bool transformed = false;
        int blend = -1, srcBlend = -1, destBlend = -1, depthTest = -1, depthWrite = -1, depthFunc = -1, cull = -1;
        GLint viewport[4] = { -1, -1, -1, -1 }, scissor[4] = { -1, -1, -1, -1 };
        float depthRange[2] = { -1, -1 };
        GLuint textures[5] = { ~0u, ~0u, ~0u, ~0u, ~0u }, samplers[5] = { ~0u, ~0u, ~0u, ~0u, ~0u };
    } bound;

    ~Executor() override
    {
        if (context == nullptr)
            return;
        for (auto &s : samplers)
            gl.DeleteSamplers(1, &s.second);
        GLuint textures[] = { backColor, backdrop, white2D, whiteCube };
        gl.DeleteTextures(4, textures);
        gl.DeleteRenderbuffers(1, &backDepth);
        gl.DeleteFramebuffers(1, &backFramebuffer);
        GLuint buffers[] = { vertexBuffer, indexBuffer, uniformBuffer, readbackBuffer };
        gl.DeleteBuffers(4, buffers);
        gl.DeleteVertexArrays(1, &vertexArray);
        gl.DeleteProgram(program3d);
        gl.DeleteProgram(programTL);
        if (readbackFence != nullptr)
            gl.DeleteSync(readbackFence);
        SDL_GL_DestroyContext(context);
    }

    // ---- device ----

    GLuint Compile(GLenum stage, const unsigned char *source)
    {
        GLuint shader = gl.CreateShader(stage);
        const char *text = (const char *)source;
        gl.ShaderSource(shader, 1, &text, NULL);
        gl.CompileShader(shader);
        GLint ok = 0;
        gl.GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048];
            gl.GetShaderInfoLog(shader, sizeof(log), NULL, log);
            Sys_Log("gfx: shader: %s", log);
            gl.DeleteShader(shader);
            return 0;
        }
        return shader;
    }

    // A program of a vertex shader and the fragment one, its uniform blocks
    // and samplers bound where Execute puts them.
    GLuint Link(const unsigned char *vertexSource, const unsigned char *fragmentSource)
    {
        GLuint vs = Compile(GL_VERTEX_SHADER, vertexSource), fs = Compile(GL_FRAGMENT_SHADER, fragmentSource);
        if (vs == 0 || fs == 0)
            return 0;
        GLuint program = gl.CreateProgram();
        gl.AttachShader(program, vs);
        gl.AttachShader(program, fs);
        gl.LinkProgram(program);
        gl.DeleteShader(vs);
        gl.DeleteShader(fs);
        GLint ok = 0;
        gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[2048];
            gl.GetProgramInfoLog(program, sizeof(log), NULL, log);
            Sys_Log("gfx: program: %s", log);
            gl.DeleteProgram(program);
            return 0;
        }
        gl.UniformBlockBinding(program, gl.GetUniformBlockIndex(program, "VertexUniforms"), VERTEX_BLOCK);
        gl.UniformBlockBinding(program, gl.GetUniformBlockIndex(program, "FragmentUniforms"), FRAGMENT_BLOCK);
        gl.UseProgram(program);
        static const char *const samplerNames[5] = { "tex0", "cube0", "tex1", "cube1", "backdrop" };
        for (int i = 0; i < 5; i++)
            gl.Uniform1i(gl.GetUniformLocation(program, samplerNames[i]), i);
        return program;
    }

    bool HasExtension(const char *name)
    {
        GLint count = 0;
        gl.GetIntegerv(GL_NUM_EXTENSIONS, &count);
        for (GLint i = 0; i < count; i++)
            if (strcmp((const char *)gl.GetStringi(GL_EXTENSIONS, i), name) == 0)
                return true;
        return false;
    }

    GLuint WhiteTexture(GLenum target)
    {
        static const uint32_t white = 0xffffffff;
        GLuint texture;
        gl.GenTextures(1, &texture);
        gl.BindTexture(target, texture);
        if (target == GL_TEXTURE_CUBE_MAP) {
            for (int face = 0; face < 6; face++)
                gl.TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA8, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE,
                              &white);
        } else {
            gl.TexImage2D(target, 0, GL_RGBA8, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, &white);
        }
        gl.TexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
        return texture;
    }

    bool CreateDevice()
    {
        window = Platform_GetWindow();
        if (window == NULL) {
            Sys_Log("gfx: no window");
            return false;
        }
        context = SDL_GL_CreateContext(window);
        if (context == nullptr) {
            Sys_Log("gfx: no OpenGL 3.3 context: %s", SDL_GetError());
            return false;
        }
        if (!gl.Load())
            return false;
        GLint major = 0, minor = 0;
        gl.GetIntegerv(GL_MAJOR_VERSION, &major);
        gl.GetIntegerv(GL_MINOR_VERSION, &minor);
        Sys_Log("gfx: OpenGL %d.%d, %s (%s)", (int)major, (int)minor, (const char *)gl.GetString(GL_RENDERER),
                (const char *)gl.GetString(GL_VERSION));
        if (major < 3 || (major == 3 && minor < 3)) {
            Sys_Log("gfx: OpenGL 3.3 is needed");
            return false;
        }
        anisotropy = Platform_GetSettingInt("video.anisotropy", 1);
        smoothScaling = Platform_GetSettingInt("video.smooth_scaling", 1);
        bcSupported = HasExtension("GL_EXT_texture_compression_s3tc");
        if (anisotropy > 1 &&
            (HasExtension("GL_EXT_texture_filter_anisotropic") || HasExtension("GL_ARB_texture_filter_anisotropic")))
            gl.GetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        gl.GetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformAlignment);

        program3d = Link(g_fixed3d_vert_glsl, g_fixed_frag_glsl);
        programTL = Link(g_fixedtl_vert_glsl, g_fixed_frag_glsl);
        if (program3d == 0 || programTL == 0)
            return false;
        white2D = WhiteTexture(GL_TEXTURE_2D);
        whiteCube = WhiteTexture(GL_TEXTURE_CUBE_MAP);
        gl.GenVertexArrays(1, &vertexArray);
        gl.BindVertexArray(vertexArray);
        GLuint buffers[4];
        gl.GenBuffers(4, buffers);
        vertexBuffer = buffers[0];
        indexBuffer = buffers[1];
        uniformBuffer = buffers[2];
        readbackBuffer = buffers[3];
        // The index buffer belongs to the vertex array.
        gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
        gl.PixelStorei(GL_PACK_ALIGNMENT, 1);
        gl.Enable(GL_SCISSOR_TEST);
        // Direct3D's front faces are clockwise; drawn upside down they wind
        // counter-clockwise.
        gl.FrontFace(GL_CCW);
        GLenum error = gl.GetError();
        if (error != GL_NO_ERROR)
            Sys_Log("gfx: OpenGL error %#x setting up", (unsigned)error);
        return true;
    }

    bool SetVideoMode(int width, int height, bool fullscreen) override
    {
        if (context == nullptr && !CreateDevice())
            return false;
        if (backFramebuffer == 0 || backWidth != width || backHeight != height) {
            if (backFramebuffer == 0) {
                gl.GenFramebuffers(1, &backFramebuffer);
                gl.GenTextures(1, &backColor);
                gl.GenTextures(1, &backdrop);
                gl.GenRenderbuffers(1, &backDepth);
            }
            gl.BindTexture(GL_TEXTURE_2D, backdrop);
            gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            gl.BindTexture(GL_TEXTURE_2D, backColor);
            gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
            gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            gl.BindRenderbuffer(GL_RENDERBUFFER, backDepth);
            gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            gl.BindFramebuffer(GL_FRAMEBUFFER, backFramebuffer);
            gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backColor, 0);
            gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, backDepth);
            bound.framebuffer = backFramebuffer;
            InvalidateTextureBinding();
            if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                Sys_Log("gfx: back buffer %dx%d incomplete", width, height);
                return false;
            }
            backWidth = width;
            backHeight = height;
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

    GlTexture *Ensure(GfxTexture *t)
    {
        if (t->gpu != NULL)
            return (GlTexture *)t->gpu;
        GlTexture *g = new GlTexture;
        g->compressed = (t->format == GFX_FORMAT_BC1 || t->format == GFX_FORMAT_BC3) && bcSupported;
        gl.GenTextures(1, &g->texture);
        GLenum target = (t->flags & GFX_TEXTURE_CUBE) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        gl.BindTexture(target, g->texture);
        gl.TexParameteri(target, GL_TEXTURE_MAX_LEVEL, t->mipLevels - 1);
        InvalidateTextureBinding();
        if (t->flags & GFX_TEXTURE_RENDER_TARGET) {
            // Storage now, so that it can be drawn into before any upload.
            Upload(t, g);
            gl.GenRenderbuffers(1, &g->depth);
            gl.BindRenderbuffer(GL_RENDERBUFFER, g->depth);
            gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, t->width, t->height);
        }
        t->gpu = g;
        t->dirty = true;
        return g;
    }

    // A level as uploaded: the CPU copy converted to BGRA (or kept as BC
    // blocks where OpenGL takes them).
    void ConvertLevel(GfxTexture *t, GlTexture *g, const uint8_t *src, int w, int h, std::vector<uint8_t> &out)
    {
        size_t n = (size_t)w * h;
        if (g->compressed) {
            out.assign(src, src + Gfx_LevelSize(t->format, w, h, NULL));
            return;
        }
        out.resize(n * 4);
        uint8_t *dst = out.data();
        switch (t->format) {
        case GFX_FORMAT_BGRX8:
            for (size_t i = 0; i < n; i++)
                ((uint32_t *)dst)[i] = ((const uint32_t *)src)[i] | 0xff000000;
            break;
        case GFX_FORMAT_BUMP_DUDVL:
            // dU, dV, luminance in red, green, blue: as the SDL_GPU executor
            // uploads them (R8G8B8A8), which the shader reads.
            for (size_t i = 0; i < n; i++) {
                dst[i * 4] = src[i * 3 + 2];
                dst[i * 4 + 1] = src[i * 3 + 1];
                dst[i * 4 + 2] = src[i * 3];
                dst[i * 4 + 3] = 0xff;
            }
            break;
        case GFX_FORMAT_BC1:
        case GFX_FORMAT_BC3: {
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
            break;
        }
        default:
            memcpy(dst, src, n * 4);
            break;
        }
    }

    // Every level of every face of a texture from its CPU copy.
    void Upload(GfxTexture *t, GlTexture *g)
    {
        bool cube = (t->flags & GFX_TEXTURE_CUBE) != 0;
        GLenum target = cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        gl.ActiveTexture(GL_TEXTURE0);
        gl.BindTexture(target, g->texture);
        InvalidateTextureBinding();
        GLenum compressedFormat =
            t->format == GFX_FORMAT_BC1 ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        std::vector<uint8_t> level;
        for (int f = 0; f < (cube ? 6 : 1); f++) {
            GLenum face = cube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + f : GL_TEXTURE_2D;
            for (int m = 0; m < t->mipLevels; m++) {
                int w = std::max(1, t->width >> m), h = std::max(1, t->height >> m);
                const std::vector<uint8_t> &cpu = t->levels[f][m];
                if (cpu.empty()) {
                    gl.TexImage2D(face, m, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
                    continue;
                }
                ConvertLevel(t, g, cpu.data(), w, h, level);
                if (g->compressed)
                    gl.CompressedTexImage2D(face, m, compressedFormat, w, h, 0, (GLsizei)level.size(), level.data());
                else if (g->specified)
                    gl.TexSubImage2D(face, m, 0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, level.data());
                else
                    gl.TexImage2D(face, m, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, level.data());
            }
        }
        g->specified = true;
    }

    void UploadTextures(std::vector<GfxTexture *> &textures)
    {
        for (GfxTexture *t : textures) {
            if (!t->dirty)
                continue;
            Upload(t, Ensure(t));
            t->dirty = false;
        }
    }

    void DestroyTexture(GfxTexture *t) override
    {
        GlTexture *g = (GlTexture *)t->gpu;
        if (g == NULL || context == nullptr)
            return;
        gl.DeleteTextures(1, &g->texture);
        gl.DeleteRenderbuffers(1, &g->depth);
        for (GLuint &framebuffer : g->framebuffers)
            if (framebuffer != 0) {
                if (bound.framebuffer == framebuffer)
                    bound.framebuffer = ~0u;
                gl.DeleteFramebuffers(1, &framebuffer);
            }
        InvalidateTextureBinding();
        delete g;
        t->gpu = NULL;
    }

    void ReadTexture(GfxTexture *t) override
    {
        GlTexture *g = (GlTexture *)t->gpu;
        if (g == NULL || t->format != GFX_FORMAT_BGRX8 && t->format != GFX_FORMAT_BGRA8)
            return;
        bool cube = (t->flags & GFX_TEXTURE_CUBE) != 0;
        gl.ActiveTexture(GL_TEXTURE0);
        gl.BindTexture(cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, g->texture);
        InvalidateTextureBinding();
        for (int f = 0; f < (cube ? 6 : 1); f++)
            gl.GetTexImage(cube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + f : GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE,
                           t->levels[f][0].data());
    }

    void InvalidateTextureBinding()
    {
        for (GLuint &texture : bound.textures)
            texture = ~0u;
    }

    // The framebuffer drawing into a texture (or a face of a cube).
    GLuint Framebuffer(GfxTexture *target)
    {
        GfxTexture *root = target->cube != NULL ? target->cube : target;
        GlTexture *g = Ensure(root);
        int face = target->cube != NULL ? target->face : 0;
        GLuint &framebuffer = g->framebuffers[face];
        if (framebuffer == 0) {
            gl.GenFramebuffers(1, &framebuffer);
            gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    root->flags & GFX_TEXTURE_CUBE ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face
                                                                   : GL_TEXTURE_2D,
                                    g->texture, 0);
            gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g->depth);
            if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                Sys_Log("gfx: render target %dx%d incomplete", root->width, root->height);
            bound.framebuffer = framebuffer;
        }
        return framebuffer;
    }

    // ---- samplers ----

    GLuint Sampler(const GfxStageState &stage)
    {
        uint32_t key = (uint32_t)(stage.minFilter & 3) | (uint32_t)(stage.magFilter & 3) << 2 |
                       (uint32_t)(stage.mipFilter & 3) << 4 | (uint32_t)(stage.address & 3) << 6;
        auto it = samplers.find(key);
        if (it != samplers.end())
            return it->second;
        GLuint s;
        gl.GenSamplers(1, &s);
        bool linear = stage.minFilter == GFX_FILTER_LINEAR;
        GLint minFilter;
        if (stage.mipFilter == GFX_MIPFILTER_NONE)
            minFilter = linear ? GL_LINEAR : GL_NEAREST;
        else if (stage.mipFilter == GFX_MIPFILTER_LINEAR)
            minFilter = linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
        else
            minFilter = linear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
        gl.SamplerParameteri(s, GL_TEXTURE_MIN_FILTER, minFilter);
        gl.SamplerParameteri(s, GL_TEXTURE_MAG_FILTER, stage.magFilter == GFX_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST);
        GLint wrap = stage.address == GFX_ADDRESS_CLAMP    ? GL_CLAMP_TO_EDGE
                     : stage.address == GFX_ADDRESS_MIRROR ? GL_MIRRORED_REPEAT
                                                           : GL_REPEAT;
        gl.SamplerParameteri(s, GL_TEXTURE_WRAP_S, wrap);
        gl.SamplerParameteri(s, GL_TEXTURE_WRAP_T, wrap);
        gl.SamplerParameteri(s, GL_TEXTURE_WRAP_R, wrap);
        if (maxAnisotropy > 1.0f && stage.mipFilter != GFX_MIPFILTER_NONE)
            gl.SamplerParameterf(s, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min((float)anisotropy, maxAnisotropy));
        samplers[key] = s;
        return s;
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

    // As in gfx_gpu.cpp: point-filtered sprites and text, read through a
    // linear sampler by the shader.
    bool ScaledPoint(const GfxStageState &stage)
    {
        return TextureKind(stage) == 1 && stage.magFilter == GFX_FILTER_POINT && stage.mipFilter == GFX_MIPFILTER_NONE;
    }

    // The two stages' textures and samplers: units 0 and 2 for 2D textures,
    // 1 and 3 for cubes.
    void BindTextures(const GfxDrawState &s)
    {
        for (int i = 0; i < 2; i++) {
            const GfxStageState &stage = s.stages[i];
            GfxTexture *t = Gfx_DrawnTexture(stage.texture);
            GlTexture *g = t != NULL ? Ensure(t) : NULL;
            GfxStageState sampled = stage;
            if (ScaledPoint(stage))
                sampled.minFilter = sampled.magFilter = GFX_FILTER_LINEAR;
            if (TextureKind(stage) == 4) {
                sampled.minFilter = sampled.magFilter = GFX_FILTER_LINEAR;
                sampled.mipFilter = GFX_MIPFILTER_LINEAR;
            }
            GLuint sampler = Sampler(sampled);
            bool cube = t != NULL && (t->flags & GFX_TEXTURE_CUBE);
            GLuint textures[2] = { g != NULL && !cube ? g->texture : white2D, g != NULL && cube ? g->texture : whiteCube };
            for (int k = 0; k < 2; k++) {
                int unit = i * 2 + k;
                if (bound.textures[unit] != textures[k]) {
                    gl.ActiveTexture(GL_TEXTURE0 + unit);
                    gl.BindTexture(k == 0 ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP, textures[k]);
                    bound.textures[unit] = textures[k];
                }
                if (bound.samplers[unit] != sampler) {
                    gl.BindSampler(unit, sampler);
                    bound.samplers[unit] = sampler;
                }
            }
        }
        // Unit 4: the backdrop of the distance fade, read by texel.
        if (bound.textures[4] != backdrop) {
            gl.ActiveTexture(GL_TEXTURE0 + 4);
            gl.BindTexture(GL_TEXTURE_2D, backdrop);
            bound.textures[4] = backdrop;
        }
        GLuint sampler = Sampler(GfxStageState());
        if (bound.samplers[4] != sampler) {
            gl.BindSampler(4, sampler);
            bound.samplers[4] = sampler;
        }
    }

    // ---- uniforms (as gfx_gpu.cpp's) ----

    static bool SameVertexUniforms(const GfxDrawState &a, const GfxDrawState &b)
    {
        for (int i = 0; i < 2; i++)
            if (a.stages[i].texCoordIndex != b.stages[i].texCoordIndex ||
                a.stages[i].textureTransform != b.stages[i].textureTransform)
                return false;
        return a.viewport.width == b.viewport.width && a.viewport.height == b.viewport.height &&
               a.screenW == b.screenW && a.screenH == b.screenH && a.lighting == b.lighting &&
               a.colorVertex == b.colorVertex && a.specular == b.specular && a.localViewer == b.localViewer &&
               a.normalizeNormals == b.normalizeNormals && a.diffuseSource == b.diffuseSource &&
               a.specularSource == b.specularSource && a.ambientSource == b.ambientSource &&
               a.emissiveSource == b.emissiveSource && a.fog == b.fog && a.fogTableMode == b.fogTableMode &&
               a.fogVertexMode == b.fogVertexMode && a.fogStart == b.fogStart && a.fogEnd == b.fogEnd &&
               a.ambient == b.ambient && memcmp(&a.world, &b.world, sizeof(a.world)) == 0 &&
               memcmp(&a.view, &b.view, sizeof(a.view)) == 0 &&
               memcmp(&a.projection, &b.projection, sizeof(a.projection)) == 0 &&
               memcmp(a.texture, b.texture, sizeof(a.texture)) == 0 &&
               memcmp(&a.material, &b.material, sizeof(a.material)) == 0 &&
               memcmp(a.lightEnabled, b.lightEnabled, sizeof(a.lightEnabled)) == 0 &&
               memcmp(a.lights, b.lights, sizeof(a.lights)) == 0;
    }

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

    void FillVertexUniforms(const GfxDrawState &s, int targetW, int targetH, const GfxViewport &viewport,
                            bool scaled, VertexUniforms &u)
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
        u.target[0] = (float)(scaled ? s.screenW : targetW);
        u.target[1] = (float)(scaled ? s.screenH : targetH);
        u.target[2] = viewport.width ? 1.0f / viewport.width : 0.0f;
        u.target[3] = viewport.height ? -1.0f / viewport.height : 0.0f;
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

    GLintptr AppendUniforms(const void *data, size_t size)
    {
        size_t offset = (uniformData.size() + uniformAlignment - 1) / uniformAlignment * uniformAlignment;
        uniformData.resize(offset + size);
        memcpy(&uniformData[offset], data, size);
        return (GLintptr)offset;
    }

    // Every draw's uniform blocks, written once for the frame: a block is
    // written when it differs from the previous draw's, as SDL_GPU pushes
    // them, and the frame's are uploaded in one go.
    void BuildUniforms(const GfxFrame &frame)
    {
        uniformData.clear();
        drawUniforms.assign(frame.commands.size(), DrawUniforms());
        GfxTexture *target = NULL;
        const GfxDrawState *vertexState = NULL;
        int vertexW = 0, vertexH = 0;
        bool vertexScaled = false;
        GLintptr vertexOffset = -1, fragmentOffset = -1;
        FragmentUniforms fragment;
        for (size_t i = 0; i < frame.commands.size(); i++) {
            const GfxCommand &c = frame.commands[i];
            if (c.type == GFX_CMD_TARGET) {
                target = c.target;
                continue;
            }
            if (c.type != GFX_CMD_DRAW)
                continue;
            int targetW = backWidth, targetH = backHeight;
            if (target != NULL) {
                GfxTexture *root = target->cube != NULL ? target->cube : target;
                targetW = root->width;
                targetH = root->height;
            }
            const GfxDrawState &s = frame.states[c.state];
            const bool scaled = target == NULL && s.screenW > 0 && s.screenH > 0;
            if (vertexState == NULL || vertexW != targetW || vertexH != targetH || vertexScaled != scaled ||
                (vertexState != &s && !SameVertexUniforms(*vertexState, s))) {
                VertexUniforms u;
                FillVertexUniforms(s, targetW, targetH,
                                   ToTarget(s.viewport, s.screenW, s.screenH, target == NULL, targetW, targetH), scaled,
                                   u);
                vertexOffset = AppendUniforms(&u, sizeof(u));
                vertexState = &s;
                vertexW = targetW;
                vertexH = targetH;
                vertexScaled = scaled;
            }
            FragmentUniforms f;
            FillFragmentUniforms(s, target == NULL && !c.transformed, f);
            if (fragmentOffset < 0 || memcmp(&f, &fragment, sizeof(f)) != 0) {
                fragmentOffset = AppendUniforms(&f, sizeof(f));
                fragment = f;
            }
            drawUniforms[i].vertexOffset = vertexOffset;
            drawUniforms[i].fragmentOffset = fragmentOffset;
        }
        gl.BindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
        gl.BufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)uniformData.size(), uniformData.data(), GL_STREAM_DRAW);
    }

    // ---- geometry ----

    void UploadGeometry(const GfxFrame &frame)
    {
        if (!frame.vertices.empty()) {
            gl.BindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            gl.BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)frame.vertices.size(), frame.vertices.data(), GL_STREAM_DRAW);
            if (bound.vertices == vertexBuffer)
                bound.vertices = ~0u;
        }
        if (!frame.indices.empty())
            gl.BufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(frame.indices.size() * 2), frame.indices.data(),
                          GL_STREAM_DRAW);
    }

    void UploadVertexBuffers(const GfxFrame &frame)
    {
        for (GfxVertexBuffer *b : frame.buffers) {
            GlVertexBuffer *g = (GlVertexBuffer *)b->gpu;
            if (g == NULL) {
                b->gpu = g = new GlVertexBuffer;
                gl.GenBuffers(1, &g->buffer);
            }
            if (g->version == b->version || b->vertices.empty())
                continue;
            gl.BindBuffer(GL_ARRAY_BUFFER, g->buffer);
            gl.BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(b->vertices.size() * sizeof(GfxVertex)), b->vertices.data(),
                          GL_STATIC_DRAW);
            g->version = b->version;
            if (bound.vertices == g->buffer)
                bound.vertices = ~0u;
        }
    }

    void DestroyVertexBuffer(GfxVertexBuffer *b) override
    {
        GlVertexBuffer *g = (GlVertexBuffer *)b->gpu;
        if (g == NULL)
            return;
        if (context != nullptr) {
            if (bound.vertices == g->buffer)
                bound.vertices = ~0u;
            gl.DeleteBuffers(1, &g->buffer);
        }
        delete g;
        b->gpu = NULL;
    }

    // The vertex format of a buffer, from its start; draws give their first
    // vertex.
    void BindVertices(GLuint buffer, bool transformed)
    {
        if (bound.vertices == buffer && bound.transformed == transformed)
            return;
        gl.BindBuffer(GL_ARRAY_BUFFER, buffer);
        if (transformed) {
            const GLsizei stride = sizeof(GfxTLVertex);
            gl.VertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (const void *)0);
            gl.VertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (const void *)16);
            gl.VertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (const void *)20);
            gl.VertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (const void *)24);
        } else {
            const GLsizei stride = sizeof(GfxVertex);
            gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)0);
            gl.VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const void *)12);
            gl.VertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (const void *)24);
            gl.VertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (const void *)28);
            gl.VertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, (const void *)32);
            gl.VertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, stride, (const void *)40);
        }
        for (GLuint i = 0; i < (transformed ? 4u : 6u); i++)
            gl.EnableVertexAttribArray(i);
        bound.vertices = buffer;
        bound.transformed = transformed;
    }

    // ---- state ----

    void BindFramebuffer(GLuint framebuffer)
    {
        if (bound.framebuffer == framebuffer)
            return;
        gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        bound.framebuffer = framebuffer;
    }

    void SetViewport(GLint x, GLint y, GLint w, GLint h, float minZ, float maxZ)
    {
        GLint v[4] = { x, y, w, h };
        if (memcmp(v, bound.viewport, sizeof(v)) != 0) {
            gl.Viewport(x, y, w, h);
            memcpy(bound.viewport, v, sizeof(v));
        }
        if (bound.depthRange[0] != minZ || bound.depthRange[1] != maxZ) {
            gl.DepthRange(minZ, maxZ);
            bound.depthRange[0] = minZ;
            bound.depthRange[1] = maxZ;
        }
    }

    void SetScissor(const GfxViewport &v)
    {
        GLint s[4] = { (GLint)v.x, (GLint)v.y, (GLint)v.width, (GLint)v.height };
        if (memcmp(s, bound.scissor, sizeof(s)) != 0) {
            gl.Scissor(s[0], s[1], s[2], s[3]);
            memcpy(bound.scissor, s, sizeof(s));
        }
    }

    void SetCapability(GLenum capability, int &current, bool on)
    {
        if (current == (int)on)
            return;
        if (on)
            gl.Enable(capability);
        else
            gl.Disable(capability);
        current = on;
    }

    // What SDL_GPU puts in a pipeline (gfx_gpu.cpp's PipelineKey).
    void SetDrawState(const GfxDrawState &s)
    {
        SetCapability(GL_BLEND, bound.blend, s.alphaBlend);
        if (s.alphaBlend && (bound.srcBlend != s.srcBlend || bound.destBlend != s.destBlend)) {
            gl.BlendFunc(BlendFactor(s.srcBlend), BlendFactor(s.destBlend));
            bound.srcBlend = s.srcBlend;
            bound.destBlend = s.destBlend;
        }
        SetCapability(GL_DEPTH_TEST, bound.depthTest, s.depthTest);
        int depthWrite = s.depthTest && s.depthWrite;
        if (bound.depthWrite != depthWrite) {
            gl.DepthMask(depthWrite ? GL_TRUE : GL_FALSE);
            bound.depthWrite = depthWrite;
        }
        if (s.depthTest && bound.depthFunc != s.depthFunc) {
            gl.DepthFunc(CompareFunc(s.depthFunc));
            bound.depthFunc = s.depthFunc;
        }
        int cull = s.cull == GFX_CULL_CW ? 1 : s.cull == GFX_CULL_CCW ? 2 : 0;
        if (bound.cull != cull) {
            if (cull == 0) {
                gl.Disable(GL_CULL_FACE);
            } else {
                gl.Enable(GL_CULL_FACE);
                // CULL_CW removes Direct3D's front faces, CULL_CCW its back ones.
                gl.CullFace(cull == 1 ? GL_FRONT : GL_BACK);
            }
            bound.cull = cull;
        }
    }

    // A clear of a rectangle of the target, as Direct3D's: the scissor limits
    // it, and the depth buffer is cleared even where depth writes are off.
    void Clear(const GfxCommand &c, bool backBuffer, int targetW, int targetH)
    {
        GLbitfield mask = 0;
        if (c.clearFlags & GFX_CLEAR_TARGET) {
            float rgba[4];
            Color(c.clearColor, rgba);
            gl.ClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
            mask |= GL_COLOR_BUFFER_BIT;
        }
        if (c.clearFlags & GFX_CLEAR_ZBUFFER) {
            gl.ClearDepth(c.clearZ);
            if (bound.depthWrite != 1) {
                gl.DepthMask(GL_TRUE);
                bound.depthWrite = 1;
            }
            mask |= GL_DEPTH_BUFFER_BIT;
        }
        if (mask == 0)
            return;
        SetScissor(ToTarget(c.clearViewport, c.clearScreenW, c.clearScreenH, backBuffer, targetW, targetH));
        gl.Clear(mask);
    }

    // ---- frame execution ----

    void Execute(GfxFrame &frame, std::vector<GfxTexture *> &textures, GfxReadback &readback, bool present,
                 bool vsync, bool stretch) override
    {
        if (context == nullptr)
            return;
        for (const GfxDrawState &s : frame.states)
            for (const GfxStageState &stage : s.stages)
                if (stage.texture != NULL)
                    Ensure(Gfx_DrawnTexture(stage.texture));
        for (const GfxCommand &c : frame.commands)
            if (c.type == GFX_CMD_TARGET && c.target != NULL)
                Ensure(c.target->cube != NULL ? c.target->cube : c.target);
        UploadTextures(textures);
        gl.BindVertexArray(vertexArray);
        UploadGeometry(frame);
        UploadVertexBuffers(frame);
        BuildUniforms(frame);

        GfxTexture *target = NULL;
        int targetW = backWidth, targetH = backHeight;
        BindFramebuffer(backFramebuffer);
        GLintptr boundVertexOffset = -1, boundFragmentOffset = -1;
        for (size_t i = 0; i < frame.commands.size(); i++) {
            const GfxCommand &c = frame.commands[i];
            if (c.type == GFX_CMD_TARGET) {
                target = c.target;
                if (target == NULL) {
                    BindFramebuffer(backFramebuffer);
                    targetW = backWidth;
                    targetH = backHeight;
                } else {
                    BindFramebuffer(Framebuffer(target));
                    GfxTexture *root = target->cube != NULL ? target->cube : target;
                    targetW = root->width;
                    targetH = root->height;
                }
                continue;
            }
            if (c.type == GFX_CMD_CLEAR) {
                Clear(c, target == NULL, targetW, targetH);
                continue;
            }
            if (c.type == GFX_CMD_BACKDROP) {
                if (target != NULL)
                    continue;
                gl.ActiveTexture(GL_TEXTURE0 + 4);
                gl.BindTexture(GL_TEXTURE_2D, backdrop);
                bound.textures[4] = backdrop;
                gl.CopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, backWidth, backHeight);
                continue;
            }
            const GfxDrawState &s = frame.states[c.state];
            GLuint program = c.transformed ? programTL : program3d;
            if (bound.program != program) {
                gl.UseProgram(program);
                bound.program = program;
            }
            SetDrawState(s);
            const GfxViewport viewport = ToTarget(s.viewport, s.screenW, s.screenH, target == NULL, targetW, targetH);
            if (c.transformed)
                SetViewport(0, 0, targetW, targetH, 0.0f, 1.0f);
            else
                SetViewport(viewport.x, viewport.y, viewport.width, viewport.height, viewport.minZ, viewport.maxZ);
            SetScissor(viewport);
            const DrawUniforms &u = drawUniforms[i];
            if (u.vertexOffset != boundVertexOffset) {
                gl.BindBufferRange(GL_UNIFORM_BUFFER, VERTEX_BLOCK, uniformBuffer, u.vertexOffset,
                                   sizeof(VertexUniforms));
                boundVertexOffset = u.vertexOffset;
            }
            if (u.fragmentOffset != boundFragmentOffset) {
                gl.BindBufferRange(GL_UNIFORM_BUFFER, FRAGMENT_BLOCK, uniformBuffer, u.fragmentOffset,
                                   sizeof(FragmentUniforms));
                boundFragmentOffset = u.fragmentOffset;
            }
            BindTextures(s);
            BindVertices(c.buffer != NULL ? ((GlVertexBuffer *)c.buffer->gpu)->buffer : vertexBuffer, c.transformed);
            GLint firstVertex = (GLint)(c.firstVertexByte / (c.transformed ? sizeof(GfxTLVertex) : sizeof(GfxVertex)));
            if (c.firstIndex != ~0u)
                gl.DrawElementsBaseVertex(PrimitiveMode(c.primitive), (GLsizei)c.indexCount, GL_UNSIGNED_SHORT,
                                          (const void *)(uintptr_t)(c.firstIndex * 2), firstVertex);
            else
                gl.DrawArrays(PrimitiveMode(c.primitive), firstVertex, (GLsizei)c.vertexCount);
        }
        if (present)
            Present(readback, vsync, stretch);
    }

    void Present(GfxReadback &readback, bool vsync, bool stretch)
    {
        // Read back before the frame is shown (the lens flare test), into a
        // pixel buffer collected when the game reads it.
        CollectReadback(readback);
        BindFramebuffer(backFramebuffer);
        if (readback.requested && readback.width > 0 && readback.height > 0) {
            int x = std::max(0, std::min(readback.x, backWidth - 1));
            int y = std::max(0, std::min(readback.y, backHeight - 1));
            int w = std::min(readback.width, backWidth - x), h = std::min(readback.height, backHeight - y);
            gl.BindBuffer(GL_PIXEL_PACK_BUFFER, readbackBuffer);
            gl.BufferData(GL_PIXEL_PACK_BUFFER, (GLsizeiptr)w * h * 4, NULL, GL_STREAM_READ);
            gl.ReadPixels(x, y, w, h, GL_BGRA, GL_UNSIGNED_BYTE, (void *)0);
            gl.BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            readbackFence = gl.FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            readbackWidth = w;
            readbackHeight = h;
            readback.requested = false;
        }
        HashFrame();

        // A minimised or hidden window is not shown: waiting for the display's
        // refresh there can block until it is shown again (OpenGL on
        // XWayland), so the frame only keeps the pace.
        if (SDL_GetWindowFlags(window) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN)) {
            SDL_Delay(16);
            return;
        }
        int interval = vsync ? 1 : 0;
        if (swapInterval != interval) {
            SDL_GL_SetSwapInterval(interval);
            swapInterval = interval;
        }
        int sw = 0, sh = 0;
        SDL_GetWindowSizeInPixels(window, &sw, &sh);
        if (sw != presentWidth || sh != presentHeight) {
            Sys_Log("gfx: %dx%d shown in %dx%d pixels", backWidth, backHeight, sw, sh);
            presentWidth = sw;
            presentHeight = sh;
        }
        // The back buffer's first row is its top: turned over into the
        // window, whose first row is its bottom.
        PresentationRect rect = GetPresentationRect(backWidth, backHeight, sw, sh, stretch);
        gl.Disable(GL_SCISSOR_TEST);
        gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        gl.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        gl.Clear(GL_COLOR_BUFFER_BIT);
        gl.BlitFramebuffer(0, 0, backWidth, backHeight, rect.x, sh - rect.y, rect.x + rect.width,
                           sh - rect.y - rect.height, GL_COLOR_BUFFER_BIT, smoothScaling ? GL_LINEAR : GL_NEAREST);
        SDL_GL_SwapWindow(window);
        gl.Enable(GL_SCISSOR_TEST);
        bound.framebuffer = ~0u;
    }

    void CollectReadback(GfxReadback &readback) override
    {
        if (readbackFence == nullptr)
            return;
        gl.ClientWaitSync(readbackFence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
        gl.DeleteSync(readbackFence);
        readbackFence = nullptr;
        gl.BindBuffer(GL_PIXEL_PACK_BUFFER, readbackBuffer);
        const uint32_t *map = (const uint32_t *)gl.MapBufferRange(
            GL_PIXEL_PACK_BUFFER, 0, (GLsizeiptr)readbackWidth * readbackHeight * 4, GL_MAP_READ_BIT);
        if (map != NULL) {
            readback.pixels.assign(map, map + (size_t)readbackWidth * readbackHeight);
            gl.UnmapBuffer(GL_PIXEL_PACK_BUFFER);
            readback.readyWidth = readbackWidth;
            readback.readyHeight = readbackHeight;
            readback.ready = true;
        }
        gl.BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    // The back buffer's pixels for Gfx_HashFrame.
    void HashFrame()
    {
        if (!Gfx_FrameHashWanted())
            return;
        std::vector<uint8_t> pixels((size_t)backWidth * backHeight * 4);
        gl.ReadPixels(0, 0, backWidth, backHeight, GL_BGRA, GL_UNSIGNED_BYTE, pixels.data());
        Gfx_HashFrame(pixels.data(), backWidth, backHeight);
    }
};

} // namespace

GfxExecutor *Gfx_CreateGlExecutor(void)
{
    return new Executor;
}

unsigned long long Gfx_PrepareGlWindow(void)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // The game draws into its own framebuffer: the window's needs no depth.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    if (Platform_GetSettingInt("video.gpu_debug", 0))
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    return SDL_WINDOW_OPENGL;
}
