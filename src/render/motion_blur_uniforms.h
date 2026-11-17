#ifndef OPENCMR2_MOTION_BLUR_UNIFORMS_H
#define OPENCMR2_MOTION_BLUR_UNIFORMS_H
// The ONE packer for the MotionBlurUniforms std140 block (shaders/motionblur.frag). Both
// executors (SDL_GPU in gfx_gpu.cpp, GL in gl_effects_executor.h) call this, like
// fog_uniforms.h: hand-copied fills drift from the shader. Layout: mix, carKeep,
// carFeather, pad, then carRect (x0 y0 x1 y1) = 2 vec4 = 8 floats. Every float is
// written explicitly - no zero-init reliance - so the pushed block never carries
// stale or unwritten lanes.
//
// carKeep/carRect: the followed car's screen rectangle from its own projected vertices
// (render/car_region.h). The shader scales the previous-frame mix to 0 inside the
// feathered rectangle, so the car stays sharp while the world blurs. The GL renderer
// records no car region: it packs keepCar = 0 and the gate stays off there.

static const int kMotionBlurUniformFloats = 8;

struct MotionBlurUniformParams {
    float mix;         // 0..1 previous-frame blend (already clamped by MotionBlurClampMix)
    int keepCar;       // requested: scale the mix to 0 inside the car rectangle
    int regionValid;   // the rectangle below is a real projected car region
    int interiorView;  // the followed view is the interior camera: the exterior windscreen mesh
                       // keeps the car's objectId, so the region is valid there - gate off anyway
    float u0, v0, u1, v1;  // car screen rectangle (uv, v downwards), valid only when regionValid
    float feather;     // soft edge of the rectangle, in uv units (dof.frag uses 0.02)
};

inline void MotionBlurUniforms_Pack(float *out, const MotionBlurUniformParams &p) {
    // A stale rectangle must never gate: keep resolves off without a valid region or in the
    // interior view, and the rect lanes are zeroed whenever the gate is off.
    const bool keep = p.keepCar != 0 && p.regionValid != 0 && p.interiorView == 0;
    out[0] = p.mix;
    out[1] = keep ? 1.0f : 0.0f;
    out[2] = p.feather;
    out[3] = 0.0f;
    out[4] = keep ? p.u0 : 0.0f;
    out[5] = keep ? p.v0 : 0.0f;
    out[6] = keep ? p.u1 : 0.0f;
    out[7] = keep ? p.v1 : 0.0f;
}
#endif
