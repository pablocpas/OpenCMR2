// OpenCMR2: motion blur uniforms — the ONE packer for the MotionBlurUniforms
// std140 block (render/shaders/motionblur.frag), shared by both executors
// (SDL_GPU in gfx_gpu.cpp, GL in gl_effects_executor.h). The layout and the
// keep/rect consistency rules are pinned here: a stale screen rectangle must
// never gate the blur, and every lane is written explicitly.
#include "render/motion_blur_uniforms.h"
#include <cstdio>

static int failures;
static void Check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

int main(void)
{
    Check(kMotionBlurUniformFloats == 8, "block is 8 floats (32-byte std140)");

    // Gate on with a valid rectangle: mix, keep, feather, then the rect lanes.
    MotionBlurUniformParams p;
    p.mix = 0.5f; p.keepCar = 1; p.regionValid = 1;
    p.u0 = 0.3f; p.v0 = 0.55f; p.u1 = 0.7f; p.v1 = 0.9f; p.feather = 0.02f;
    float u[kMotionBlurUniformFloats];
    MotionBlurUniforms_Pack(u, p);
    Check(u[0] == 0.5f && u[1] == 1.0f && u[2] == 0.02f && u[3] == 0.0f, "header lanes: mix, keep, feather, pad");
    Check(u[4] == 0.3f && u[5] == 0.55f && u[6] == 0.7f && u[7] == 0.9f, "rect lanes");

    // Gate requested off: rect lanes are zeroed so a stale rectangle can never gate.
    p.keepCar = 0;
    MotionBlurUniforms_Pack(u, p);
    Check(u[0] == 0.5f && u[1] == 0.0f, "keep off keeps the mix");
    Check(u[4] == 0.0f && u[5] == 0.0f && u[6] == 0.0f && u[7] == 0.0f, "keep off zeroes the rect");

    // No valid car region (cockpit camera): the gate resolves off even when requested.
    p.keepCar = 1; p.regionValid = 0;
    MotionBlurUniforms_Pack(u, p);
    Check(u[1] == 0.0f && u[6] == 0.0f, "invalid region disables the gate");

    // Interior (cockpit) view: the exterior windscreen mesh keeps the followed car's objectId,
    // so the projected region is VALID there - the gate must still resolve off, or the world
    // behind the glass loses its blur (PR review finding).
    p.keepCar = 1; p.regionValid = 1; p.interiorView = 1;
    MotionBlurUniforms_Pack(u, p);
    Check(u[1] == 0.0f && u[4] == 0.0f && u[6] == 0.0f, "interior view disables the gate even with a valid region");
    p.interiorView = 0;
    MotionBlurUniforms_Pack(u, p);
    Check(u[1] == 1.0f && u[4] == 0.3f, "exterior view gates again");

    // Zero mix still packs (the pass is skipped by the caller, but the block stays well-formed).
    p.mix = 0.0f; p.keepCar = 1; p.regionValid = 1;
    MotionBlurUniforms_Pack(u, p);
    Check(u[0] == 0.0f && u[1] == 1.0f, "zero mix packs");

    if (failures == 0) {
        puts("motion blur uniforms: packer layout and keep/rect rules hold");
        return 0;
    }
    return 1;
}
