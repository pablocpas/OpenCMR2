#ifndef OPENCMR2_FOG_DYNAMICS_H
#define OPENCMR2_FOG_DYNAMICS_H
#include <cmath>
// Optional dynamic fog (video.fog_spatial / video.fog_evolve, defaults off): scales
// video.fog_amount along the stage route. Visual only: it only changes the post-process
// fog amount the shader sees; it never touches physics, the route, particles or the RNG.
// Progress is the viewed car's route-node index, which lives in replay snapshots, so a
// replay sees the same fog sequence as the live run.
//
// video.fog_spatial (0/1, default 0): dense pockets opening into clearer stretches
// along the stage - two fixed sine waves over route progress with a per-stage phase, so
// each stage keeps its own pocket layout and every run of a stage matches.
// video.fog_evolve (0..200, default 100 = neutral): stage-wide trend from start to
// finish - below 100 the fog clears toward the end, above 100 it thickens. Route
// progress (distance), not elapsed race time: slow driving does not change the look
// sequence and replays stay deterministic.

// Spatial pockets: up to +/-50% around the base amount, always smooth.
inline float FogDynamics_SpatialMult(float progress01, float phase01) {
    float a = 0.65f * std::sin(6.2831853f * (3.f * progress01 + phase01));
    float b = 0.35f * std::sin(6.2831853f * (7.f * progress01 + 2.f * phase01));
    return 1.f + 0.5f * (a + b);
}
// Stage-wide trend: +/-50% at the endpoints at the extremes, neutral mid-stage.
inline float FogDynamics_EvolveMult(float progress01, int evolve) {
    int e = evolve < 0 ? 0 : evolve > 200 ? 200 : evolve;
    return 1.f + ((e - 100) / 200.f) * (2.f * progress01 - 1.f);
}
inline float FogDynamics_Mult(float progress01, float phase01, int spatial, int evolve) {
    float m = 1.f;
    if (spatial) m *= FogDynamics_SpatialMult(progress01, phase01);
    if (evolve != 100) m *= FogDynamics_EvolveMult(progress01, evolve);
    if (m < 0.05f) m = 0.05f;
    if (m > 2.f) m = 2.f;
    return m;
}
// Lowest multiplier the curves can reach on any stage. Fog-linked draw culling
// (video.fog_cull) must key off the base amount times this, never the current value,
// so a clearing stretch cannot reveal geometry the cull cut while it was foggy.
inline float FogDynamics_MinMult(int spatial, int evolve) {
    float m = 1.f;
    if (spatial) m *= 0.5f * 0.2f;   // route pockets (0.5) times the shader's world-bank floor (0.2, fog.frag fogBank)
    if (evolve != 100) {
        int e = evolve < 0 ? 0 : evolve > 200 ? 200 : evolve;
        m *= 1.f - std::fabs((e - 100) / 200.f);
    }
    if (m < 0.05f) m = 0.05f;
    return m;
}
// Effective amount last applied by FogDynamics_Step (0 = dynamics not driving the fog),
// for the OSD fog line. Defined in CarEffects.cpp.
extern int g_fogDynamicsEffective;
void FogDynamics_Step(int step16_16);
#endif
