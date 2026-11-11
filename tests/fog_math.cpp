#include <initializer_list>
#include "render/fog_math.h"
#include <cstdio>
#include <cmath>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    // Density clamps to 0..200 percent; 100 = the deliberate heavy starting look.
    CHECK(Fog_Density(-5) == 0.f);
    CHECK(Fog_Density(0) == 0.f);
    CHECK(std::fabs(Fog_Density(100) - 0.03f) < 1e-7f);
    CHECK(std::fabs(Fog_Density(200) - 0.06f) < 1e-7f);
    CHECK(std::fabs(Fog_Density(900) - 0.06f) < 1e-7f);
    // At 100 percent the curve hits its documented anchors (23 units ~50%, 60 ~84%, 150 ~99%).
    float d = Fog_Density(100);
    CHECK(std::fabs(Fog_Factor(23.f, d) - 0.50f) < 0.02f);
    CHECK(std::fabs(Fog_Factor(60.f, d) - 0.83f) < 0.02f);
    CHECK(Fog_Factor(150.f, d) > 0.97f);
    CHECK(Fog_Factor(0.f, d) == 0.f && Fog_Factor(-3.f, d) == 0.f && Fog_Factor(10.f, 0.f) == 0.f);
    CHECK(Fog_Factor(50.f, Fog_Density(200)) > Fog_Factor(50.f, Fog_Density(50)));
    // ViewZ round trip: depth from near/far recovers the distance (the dof.frag formula).
    for (float nearZ : {0.1f, 0.5f, 1.0f}) for (float farZ : {150.f, 400.f}) {
        for (float z = nearZ * 1.01f; z < farZ; z *= 1.7f) {
            float depth = farZ / (farZ - nearZ) * (1.f - nearZ / z);
            float got = Fog_ViewZ(depth, nearZ, farZ);
            CHECK(std::fabs(got - z) / z < 0.001f);
        }
    }
    CHECK(Fog_ViewZ(0.5f, 0.f, 0.f) == 0.f);          // unknown projection
    CHECK(Fog_ViewZ(0.5f, 2.f, 1.f) == 0.f);          // far <= near rejected
    // GL half-range buffer: the same distances survive the 0.5+0.5*d packing the
    // GL depth buffer applies to the game's D3D-style NDC z.
    for (float nearZ : {0.1f, 0.5f, 1.0f}) for (float farZ : {150.f, 400.f}) {
        for (float z = nearZ * 1.01f; z < farZ; z *= 1.7f) {
            float depth = farZ / (farZ - nearZ) * (1.f - nearZ / z);
            float glBuffer = 0.5f + 0.5f * depth;
            float got = Fog_ViewZGlBuffer(glBuffer, nearZ, farZ);
            CHECK(std::fabs(got - z) / z < 0.001f);
        }
    }
    CHECK(Fog_ViewZGlBuffer(0.75f, 0.f, 0.f) == 0.f); // unknown projection stays off
    // Projection voting (GL parity with the SDL_GPU ResolveProjection rule):
    // exactly one distinct near/far pair among the depth writers resolves;
    // a mixed-projection frame (e.g. the unsorted-node pass with its own far
    // plane) is ambiguous and keeps the raw-depth fallback instead of guessing.
    {
        FogProjVote votes[4] = {};
        float n = -1.f, f = -1.f;
        CHECK(Fog_ResolveProjectionVote(votes, &n, &f) == 0);            // empty
        for (int i = 0; i < 30; ++i) Fog_VoteProjection(votes, 0.1f, 150.f);
        CHECK(Fog_ResolveProjectionVote(votes, &n, &f) == 1 && n == 0.1f && f == 150.f);
        Fog_VoteProjection(votes, 0.1f, 100.f);                          // second distinct pair: ambiguous
        n = f = -1.f;
        CHECK(Fog_ResolveProjectionVote(votes, &n, &f) == 0);
        FogProjVote near2[4] = {};
        Fog_VoteProjection(near2, 0.1f, 150.f);
        Fog_VoteProjection(near2, 0.1f * 1.0005f, 150.f);                // within 1e-3: same pair
        CHECK(Fog_ResolveProjectionVote(near2, &n, &f) == 1);
        FogProjVote full[4] = {};
        for (int i = 0; i < 4; ++i) Fog_VoteProjection(full, 0.1f, 100.f + 50.f * i);
        Fog_VoteProjection(full, 0.1f, 500.f);                           // fifth pair dropped, still ambiguous
        CHECK(Fog_ResolveProjectionVote(full, &n, &f) == 0);
    }
    // Probe verdict: projection state gates first, then pollution, curve strength, output shift.
    CHECK(Fog_ProbeVerdict(false, 1.0f, 0.9f, 0.0f) == 5);
    CHECK(Fog_ProbeVerdict(true, 1.0f, 0.9f, 0.0f) == 1);    // near depth wins over every later check
    CHECK(Fog_ProbeVerdict(true, 2.99f, 0.0f, 0.0f) == 1);
    CHECK(Fog_ProbeVerdict(true, 3.0f, 0.1f, 0.0f) == 2);    // far, but the curve predicts almost nothing
    CHECK(Fog_ProbeVerdict(true, 50.0f, 0.14f, 0.5f) == 2);
    CHECK(Fog_ProbeVerdict(true, 50.0f, 0.5f, 0.01f) == 3);  // expected fog, no output shift
    CHECK(Fog_ProbeVerdict(true, 50.0f, 0.5f, 0.02f) == 4);  // shift at the threshold: active
    CHECK(Fog_ProbeVerdict(true, 50.0f, 0.9f, 0.6f) == 4);
    // Raw-depth fallback: nothing below depth 0.990, heavier toward the far plane, zero density zero.
    CHECK(Fog_FactorRawDepth(0.989f, 0.03f) == 0.f);
    CHECK(Fog_FactorRawDepth(1.0f, 0.f) == 0.f);
    CHECK(Fog_FactorRawDepth(0.995f, 0.03f) > 0.f);
    CHECK(Fog_FactorRawDepth(0.995f, 0.03f) < Fog_FactorRawDepth(1.0f, 0.03f));
    CHECK(Fog_FactorRawDepth(1.0f, 0.03f) < 1.f);     // exponential never quite reaches 1
    // Height gradient: 0.55 at the top, 1.45 at the bottom, monotonic between.
    CHECK(std::fabs(Fog_HeightScale(0.f) - 0.55f) < 1e-6f);
    CHECK(std::fabs(Fog_HeightScale(1.f) - 1.45f) < 1e-6f);
    CHECK(Fog_HeightScale(0.3f) < Fog_HeightScale(0.6f));
    // Sky cap applies only at the far plane.
    CHECK(Fog_SkyCap(0.99f, 1.0f) == 0.85f);
    CHECK(Fog_SkyCap(0.99f, 0.9f) == 0.99f);
    CHECK(Fog_SkyCap(0.5f, 1.0f) == 0.5f);
    // Sun glow: 1 at the sun, 0 at and beyond about 0.625 aspect-corrected screen heights, cubic between.
    CHECK(std::fabs(Fog_SunGlow(0.f, 0.f, 1.f) - 1.f) < 1e-6f);
    CHECK(Fog_SunGlow(0.625f, 0.f, 1.f) == 0.f);
    CHECK(Fog_SunGlow(1.f, 0.f, 1.f) == 0.f);
    CHECK(Fog_SunGlow(0.3f, 0.f, 1.f) > 0.f && Fog_SunGlow(0.3f, 0.f, 1.f) < 1.f);
    // Car-light glow: full at the light, zero at/past the radius, quadratic falloff, no negative.
    CHECK(std::fabs(Fog_LightGlow(0.f, 0.f, 1.f, 0.18f) - 1.f) < 1e-6f);
    CHECK(Fog_LightGlow(0.18f, 0.f, 1.f, 0.18f) == 0.f);
    CHECK(Fog_LightGlow(0.3f, 0.f, 1.f, 0.18f) == 0.f);
    CHECK(Fog_LightGlow(0.09f, 0.f, 1.f, 0.18f) > 0.5f && Fog_LightGlow(0.09f, 0.f, 1.f, 0.18f) < 0.6f);
    CHECK(Fog_LightGlow(0.f, 0.f, 0.f, 0.18f) == 0.f);
    CHECK(Fog_LightGlow(0.f, 0.f, 1.f, 0.f) == 0.f);
    CHECK(Fog_LightGlow(0.05f, 0.05f, 0.5f, 0.18f) > 0.f && Fog_LightGlow(0.05f, 0.05f, 0.5f, 0.18f) < 0.5f);
    CHECK(Fog_SunGlow(0.f, 0.f, 0.f) == 0.f);
    CHECK(Fog_SunGlow(0.1f, 0.f, 0.5f) < Fog_SunGlow(0.1f, 0.f, 1.f));
    // Colour: cool at zero glow, warm at full glow, channels inside 0..1.
    float r0, g0, b0, r1, g1, b1;
    Fog_Color(0.f, &r0, &g0, &b0); Fog_Color(1.f, &r1, &g1, &b1);
    CHECK(b0 > r0);                  // cool
    CHECK(r1 > b1);                  // warm
    for (float t : {0.f, .25f, .5f, .75f, 1.f, 1.5f, -0.5f}) {
        float r, g, b; Fog_Color(t, &r, &g, &b);
        CHECK(r >= 0.f && r <= 1.f && g >= 0.f && g <= 1.f && b >= 0.f && b <= 1.f);
    }
    // Clear distance: off means full curve; on means 0 at the camera, smoothstep to the
    // full curve at clearDist, monotonic between (mirrors the shader's smoothstep).
    CHECK(Fog_ClearScale(10.f, 0.f) == 1.f);
    CHECK(Fog_ClearScale(10.f, -5.f) == 1.f);
    CHECK(Fog_ClearScale(0.f, 100.f) == 0.f);
    CHECK(std::fabs(Fog_ClearScale(50.f, 100.f) - 0.5f) < 1e-6f);
    CHECK(Fog_ClearScale(100.f, 100.f) == 1.f);
    CHECK(Fog_ClearScale(150.f, 100.f) == 1.f);
    CHECK(Fog_ClearScale(25.f, 100.f) < Fog_ClearScale(75.f, 100.f));
    // Fog-aware cull distance: 99.5% at the weakest gradient, conservative floors,
    // off when there is no density.
    CHECK(Fog_CullDistance(0, 0.f) == 0.f);
    CHECK(std::fabs(Fog_CullDistance(100, 0.f) - 176.6f) < 0.2f);   // 5.298/0.03
    CHECK(std::fabs(Fog_CullDistance(200, 0.f) - 150.f) < 1e-6f);   // 88 clamps to the 150 floor
    CHECK(std::fabs(Fog_CullDistance(100, 200.f) - 250.f) < 1e-6f); // clear distance floor wins
    CHECK(Fog_CullDistance(30, 0.f) > 500.f);                       // light fog barely culls
    // Light beam wedge (screen-space approximation): brightest at the lamp, decays
    // along the beam, falls off laterally, zero behind the lamp and with no intensity.
    float beamAtLamp = Fog_LightBeam(0.f, 0.f, 0.f, 1.f, 0.4f, 0.9f);
    float beamMid = Fog_LightBeam(0.f, 0.2f, 0.f, 1.f, 0.4f, 0.9f);
    CHECK(beamAtLamp > 0.8f);                          // halo at the origin (wedge ramps in)
    CHECK(beamMid > 0.f && beamMid < beamAtLamp);      // decays along the beam
    CHECK(beamMid > Fog_LightBeam(0.f, 0.4f, 0.f, 1.f, 0.4f, 0.9f));
    CHECK(Fog_LightBeam(0.f, -0.2f, 0.f, 1.f, 0.4f, 0.9f) < beamMid);  // behind the lamp: halo only
    CHECK(Fog_LightBeam(0.3f, 0.2f, 0.f, 1.f, 0.4f, 0.9f) < beamMid);  // lateral falloff
    CHECK(Fog_LightBeam(0.f, 0.2f, 0.f, 1.f, 0.4f, 0.f) == 0.f);       // no intensity no light
    // Foreshortened beam (len ~0, pointing at the camera): the halo still glows at the lamp.
    CHECK(Fog_LightBeam(0.f, 0.f, 0.f, 1.f, 0.f, 0.9f) > 0.8f);
    CHECK(Fog_LightBeam(0.2f, 0.f, 0.f, 1.f, 0.f, 0.9f) < 0.05f);      // but not away from it
    // Zero direction collapses to the halo rather than NaN.
    CHECK(Fog_LightBeam(0.f, 0.f, 0.f, 0.f, 0.4f, 0.9f) > 0.8f);
    // Soft shoulder: 0 at 0, monotonic, approaches but never reaches 1.
    CHECK(Fog_SoftShoulder(0.f) == 0.f);
    CHECK(Fog_SoftShoulder(-1.f) == 0.f);
    CHECK(Fog_SoftShoulder(0.5f) < Fog_SoftShoulder(1.0f));
    CHECK(Fog_SoftShoulder(2.f) < 1.f);
    CHECK(Fog_SoftShoulder(10.f) <= 1.f);          // float: 1-exp(-20) rounds to 1
    CHECK(Fog_SoftShoulder(100.f) <= 1.f);
    // Headroom: 1 on a black scene, 0 at full white, monotonic between, clamped 0..1.
    CHECK(Fog_LightHeadroom(0.f, 0.f, 0.f) == 1.f);
    CHECK(Fog_LightHeadroom(1.f, 1.f, 1.f) == 0.f);
    CHECK(Fog_LightHeadroom(0.9f, 0.5f, 0.2f) < Fog_LightHeadroom(0.4f, 0.5f, 0.2f));
    CHECK(Fog_LightHeadroom(2.f, 2.f, 2.f) == 0.f);
    for (float m : {0.f, .2f, .5f, .8f, 1.f})
        CHECK(Fog_LightHeadroom(m, m, m) >= 0.f && Fog_LightHeadroom(m, m, m) <= 1.f);
    // Beam occluder-edge classification (Fog_OccluderCounts, mirrors
    // occluderCounts in shaders/fog.frag).
    // GPU full-range, close foliage (Devin review case): near/far 1/250, a
    // leaf at 1.4 world units -> raw ~0.287, beam segment at 30 -> ~0.971.
    // A raw 30% band (cutoff ~0.291) would discard the leaf though it stands
    // in front of the segment; the view-space classification keeps it.
    {
        float leaf = 250.f * (1.4f - 1.f) / (1.4f * (250.f - 1.f));   // ~0.287
        float seg  = 250.f * (30.f - 1.f) / (30.f * (250.f - 1.f));   // ~0.971
        CHECK(leaf < 0.30f * seg);                          // the raw-ratio failure itself
        CHECK(Fog_OccluderCounts(leaf, seg, 1.f, 250.f, 0.f));        // full-range: counts
        CHECK(Fog_OccluderCounts(0.5f + 0.5f * leaf, 0.5f + 0.5f * seg, 1.f, 250.f, 1.f));   // GL half-range: same answer
    }
    // GPU full-range, cockpit shell: interior at 0.6 units (near 0.5) beside
    // a beam segment at 30 must NOT count - open road next to the car's own
    // shell keeps the beam.
    {
        float shell = 250.f * (0.6f - 0.5f) / (0.6f * (250.f - 0.5f));   // ~0.167
        float seg   = 250.f * (30.f - 0.5f) / (30.f * (250.f - 0.5f));   // ~0.983
        CHECK(!Fog_OccluderCounts(shell, seg, 0.5f, 250.f, 0.f));
        CHECK(!Fog_OccluderCounts(0.5f + 0.5f * shell, 0.5f + 0.5f * seg, 0.5f, 250.f, 1.f));
    }
    // Behind-segment silhouette (road 50 beside tree 10, segment 5): the tree
    // lies BEHIND the segment and must never count.
    {
        float tree = 250.f * (10.f - 1.f) / (10.f * (250.f - 1.f));   // ~0.904
        float seg  = 250.f * (5.f - 1.f) / (5.f * (250.f - 1.f));     // ~0.803
        CHECK(!Fog_OccluderCounts(tree, seg, 1.f, 250.f, 0.f));
    }
    // Projection unknown (GL recovery fallback): raw band at 30% of the
    // segment reference. Harness ortho cases: cockpit shell 0.067 beside a
    // 0.8 segment is excluded, foliage leaf 0.467 beside a 0.81+ segment
    // counts.
    CHECK(!Fog_OccluderCounts(0.067f, 0.8f, 0.f, 0.f, 0.f));
    CHECK(Fog_OccluderCounts(0.467f, 0.8135f, 0.f, 0.f, 0.f));
    // Beam depth-gate reference (Fog_BeamGateRef, mirrors the gate line in
    // shaders/fog.frag): forward beams (q < 1) raise the segment reference to
    // the biased lamp reference (own-bodywork occlusion); backward beams
    // (q >= 1) approach the camera and gate on the segment reference alone,
    // so scenery between the segment and the lamp keeps the beam's light
    // (Devin r4536372133: rear lamp biased ref far, midpoint segment near).
    CHECK(Fog_BeamGateRef(0.9995f, 0.9741f, 2.0f) == 0.9741f);   // backward: segment ref alone
    CHECK(Fog_BeamGateRef(0.9995f, 0.9741f, 1.0f) == 0.9741f);   // q == 1 is not a forward tail
    CHECK(Fog_BeamGateRef(0.9995f, 0.9741f, 0.5f) == 0.9995f);   // forward: biased lamp gate
    CHECK(Fog_BeamGateRef(0.9741f, 0.9995f, 0.5f) == 0.9995f);   // forward: farther segment wins
    if (!fails) std::puts("fog_math ok");
    // FogUniforms layout pin: shaders/fog.frag declares fog, sun, extra,
    // lights[8], lightInfo[8], beams[8] = 27 vec4; the executors' beam writes
    // reach index 76 + 7*4 + 2 = 106 with all 8 lights.
    CHECK(kFogUniformFloats == 148);
    CHECK(76 + 7 * 4 + 3 <= kFogUniformFloats);

    return fails;
}