// Exercising regression for the FogUniforms 23-vs-27-vec4 stack overflow:
// the shared packer (render/fog_uniforms.h, called by BOTH executors) runs
// with the full 8 lights against a guard, so any size drift faults or
// stomps a sentinel here instead of a native stack smash. Values are
// verified per lane, including the beam tail lanes the old 92-float
// arrays could not hold.
//
// Guard by platform: POSIX gets a real PROT_NONE page right after the
// uniform window, which faults on ANY trailing read or write, however
// large. Windows cannot include windows.h here (the project's own
// port/types.h typedefs DWORD/ULONG/UINT32, which clash with the SDK
// headers), so it uses 64-float canary fences on both sides instead:
// that catches over- and underruns that land within the fences, which
// covers the sequential size-drift writes this regression produces, but
// unlike the guard page it would miss a write that jumps past the fence.
#include "render/fog_uniforms.h"
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include <vector>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)

#ifdef _WIN32
static const int kFence = 64;
static const float kCanary = -13579.2468f;
#endif

int main() {
    CHECK(kFogUniformFloats == 148);
#ifdef _WIN32
    std::vector<float> buf(kFence + kFogUniformFloats + kFence, kCanary);
    float *u = buf.data() + kFence;
    const auto fencesOk = [&]() {
        for (int i = 0; i < kFence; i++) {
            if (buf[i] != kCanary || buf[kFence + kFogUniformFloats + i] != kCanary) return false;
        }
        return true;
    };
#else
    long page = sysconf(_SC_PAGESIZE);
    size_t span = (size_t)page * 2;
    unsigned char *base = (unsigned char *)mmap(nullptr, span, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    CHECK(base != MAP_FAILED);
    if (base == MAP_FAILED) return 1;
    CHECK(mprotect(base + page, (size_t)page, PROT_NONE) == 0);
    // Array ends exactly at the guard boundary: one float past lane 107 faults.
    float *u = (float *)(base + page - kFogUniformFloats * sizeof(float));
#endif

    GfxFogLight lights[8] = {};   // zero-init: lanes the fixture does not set (beamFade, halos) must be deterministic, not stack garbage (CI linux leg flaked on u[107])
    for (int i = 0; i < 8; i++) {
        lights[i].x = 0.10f + i; lights[i].y = 0.20f + i;
        lights[i].depthRef = 0.30f + i * 0.01f; lights[i].intensity = 0.5f + i * 0.01f;
        lights[i].r = 0.9f; lights[i].g = 0.8f; lights[i].b = 0.7f;
        lights[i].startRamp = i == 7 ? 0.33f : 0.0f; lights[i].beamX = 0.31f + i; lights[i].beamY = 0.41f + i; lights[i].beamLen = 0.51f + i;
    }
    FogUniformParams p{};
    p.density = 0.03f; p.glow = 1.0f; p.nearZ = 1.5f; p.farZ = 900.0f;
    p.sunX = 0.4f; p.sunY = 0.3f; p.sunVisible = 1.0f; p.aspect = 1.6f;
    p.clearDistance = 60.0f; p.lightStrength = 1.0f;
    p.depthHalfRange = 1.0f;

    FogUniforms_Pack(u, p, lights, 8);   // overran the old 92-float layout
    CHECK(u[0] == 0.03f && u[2] == 1.5f && u[3] == 900.0f);
    CHECK(u[8] == 60.0f && u[9] == 1.0f && u[10] == 1.0f && u[11] == 0.0f);   // extra: clear, strength, half-range flag
    CHECK(u[104] == 0.31f + 7 && u[105] == 0.41f + 7 && u[106] == 0.51f + 7);   // beams[7] tail lanes
    CHECK(u[107] == 0.0f);
    CHECK(u[12 + 7 * 4 + 3] == 0.33f && u[12 + 3] == 0.0f);   // lights[i].w carries startRamp (0 = none)
    CHECK(u[12 + 7 * 4] == 0.10f + 7 && u[44 + 7 * 4] == 0.30f + 7 * 0.01f);     // lights[7], depthRef
#ifdef _WIN32
    CHECK(fencesOk());
#endif

    // depthRef 0: no depth gate.
    std::memset(u, 0xCD, kFogUniformFloats * sizeof(float));
    lights[7].depthRef = 0.0f;
    FogUniforms_Pack(u, p, lights, 8);
    CHECK(u[44 + 7 * 4] == 0.0f && u[44 + 7 * 4 + 1] == 0.9f);
    lights[7].depthRef = 0.37f;
#ifdef _WIN32
    CHECK(fencesOk());
#endif

    // Oversized count clamps to the 8 shader lanes; fewer lights zero the rest.
    FogUniforms_Pack(u, p, lights, 99);
    CHECK(u[104] == 0.31f + 7);
    FogUniforms_Pack(u, p, lights, 2);
    CHECK(u[12 + 2 * 4] == 0.0f && u[76 + 2 * 4] == 0.0f && u[106] == 0.0f);
    FogUniforms_Pack(u, p, nullptr, 0);
#ifdef _WIN32
    CHECK(fencesOk());
#else
    munmap(base, span);
#endif

    std::printf(fails ? "FAILED\n" : "fog uniforms pack ok\n");
    return fails ? 1 : 0;
}
