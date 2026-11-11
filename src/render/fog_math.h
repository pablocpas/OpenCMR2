#ifndef OPENCMR2_FOG_MATH_H
#define OPENCMR2_FOG_MATH_H
#include <cmath>
// Optional atmosphere fog (video.fog, default off): a post-process distance fog with a
// ground-mist gradient and a sun-ward glow. Visual only: it reads the rendered scene, its
// depth and the projected sun position; it never touches game state. These pure functions
// are the reference for shaders/fog.frag and are covered by tests/fog_math.cpp.

// Density from video.fog_amount (0..200 accepted; the menu offers 0..100).
// 100 is the deliberately heavy "overdo first, then bring it down" starting look.

// FogUniforms std140 layout, shared by shaders/fog.frag and both executors
// (gfx_gpu.cpp, gl_effects_executor.h): fog, sun, extra, lights[8],
// lightInfo[8], beams[8], halos[8], camA, camB = 37 vec4 = 148 floats. The CPU arrays MUST
// be this size: halo writes reach index 108 + 7*4 + 2 = 138 with all 8 lights.
inline constexpr int kFogUniformFloats = 37 * 4;
static_assert(kFogUniformFloats >= 108 + 7 * 4 + 3, "halos[8] writes must fit the FogUniforms array");

inline float Fog_Density(int amountPercent) {
    int a = amountPercent < 0 ? 0 : amountPercent > 200 ? 200 : amountPercent;
    return a / 100.f * 0.03f;   // at 100: 23 world units -> 50% fog, 60 -> 84%, 150 -> 99%
}
// View distance from hardware depth (same formula as dof.frag). 0 = projection unknown.
inline float Fog_ViewZ(float depth, float nearZ, float farZ) {
    if (!(nearZ > 0.f) || !(farZ > nearZ)) return 0.f;
    float denom = farZ - depth * (farZ - nearZ);
    if (denom < 1e-6f) denom = 1e-6f;
    return nearZ * farZ / denom;
}
// Projection voting for whole-frame depth recovery (mirrors the SDL_GPU
// executor's ResolveProjection): a race frame can mix projections (the
// unsorted-node pass uses its own far plane), and a single guessed near/far
// then mis-recovers every pixel. Vote the distinct near/far pairs among the
// draws that wrote the sampled depth; exactly one distinct pair means the
// frame's depth is unambiguous, anything else keeps the raw-depth fallback.
struct FogProjVote { float n, f; int count; };
inline void Fog_VoteProjection(FogProjVote votes[4], float n, float f) {
    for (int i = 0; i < 4; ++i)
        if (votes[i].count && std::fabs(votes[i].n - n) < 1e-3f * n && std::fabs(votes[i].f - f) < 1e-3f * f) { votes[i].count++; return; }
    for (int i = 0; i < 4; ++i)
        if (!votes[i].count) { votes[i].n = n; votes[i].f = f; votes[i].count = 1; return; }
}
// Returns 1 and sets *outNear/*outFar when exactly one distinct pair voted;
// 0 (leave the raw-depth fallback) when ambiguous or empty.
inline int Fog_ResolveProjectionVote(const FogProjVote votes[4], float *outNear, float *outFar) {
    int best = -1, distinct = 0;
    for (int i = 0; i < 4; ++i) {
        if (votes[i].count) distinct++;
        if (votes[i].count && (best < 0 || votes[i].count > votes[best].count)) best = i;
    }
    if (distinct != 1 || best < 0) return 0;
    *outNear = votes[best].n; *outFar = votes[best].f;
    return 1;
}
// Same recovery when the buffer holds GL half-range depth: GL maps the game's
// D3D-style NDC z [0,1] into buffer [0.5,1] (clip range [-1,1] clips nothing),
// so d_game = 2*buffer - 1 before the hyperbolic recovery.
inline float Fog_ViewZGlBuffer(float bufferDepth, float nearZ, float farZ) {
    return Fog_ViewZ(bufferDepth * 2.f - 1.f, nearZ, farZ);
}
// Beam occluder-edge fade classification (mirrors occluderCounts in
// shaders/fog.frag): a depth neighbor counts as occluder evidence for a beam
// when its raw buffer depth is in front of that beam's per-pixel segment
// reference (monotone remap-safe on both executors) and it is not
// near-camera geometry. Projection known: view-space distance - closer than
// kFogOccNearCamera world units is the car's own interior (cockpit shell,
// dashboard), which blocks beams only inside its own silhouette. A raw-depth
// ratio cannot express that: full-range and half-range buffers map the same
// geometry to different ratios. Projection unknown (GL recovery fallback):
// raw band at 30% of the segment reference.
inline constexpr float kFogOccNearCamera = 1.0f;
inline bool Fog_OccluderCounts(float neighborRaw, float segRefRaw, float nearZ, float farZ, float depthHalfRange) {
    if (!(neighborRaw < segRefRaw)) return false;   // behind/at the segment: never evidence
    if (nearZ > 0.f && farZ > nearZ) {
        float vz = depthHalfRange > 0.5f ? Fog_ViewZGlBuffer(neighborRaw, nearZ, farZ)
                                         : Fog_ViewZ(neighborRaw, nearZ, farZ);
        return vz >= kFogOccNearCamera;
    }
    return neighborRaw >= segRefRaw * 0.30f;
}
// Beam depth-gate reference (mirrors the gate line in shaders/fog.frag):
// the per-pixel segment reference, raised to the biased lamp reference for
// FORWARD beams (q < 1) so the car's own bodywork in front of the lamp
// occludes its beam. A backward beam (q >= 1) approaches the camera, so its
// segment depth gets closer while the lamp's biased reference stays farther;
// taking the maximum would hide the beam's light on scenery between the
// segment and the lamp (Devin r4536372133), so backward beams gate on the
// segment reference alone.
inline float Fog_BeamGateRef(float lampRef, float segRef, float q) {
    return q < 1.f ? (lampRef > segRef ? lampRef : segRef) : segRef;
}
// Exponential distance fog on a real view distance.
inline float Fog_Factor(float viewZ, float density) {
    if (!(viewZ > 0.f) || !(density > 0.f)) return 0.f;
    return 1.f - std::exp(-density * viewZ);
}
// Optional near-field clear distance (video.fog_clear, world units, default 0 = off):
// scales the fog factor from 0 at the camera to the full curve at clearDist, so the
// road ahead stays readable while the haze beyond keeps its density. Deliberately a
// global near clearance in every direction, not a road mask.
inline float Fog_ClearScale(float viewZ, float clearDist) {
    if (!(clearDist > 0.f)) return 1.f;
    float t = viewZ / clearDist;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    return t * t * (3.f - 2.f * t);
}
// Opt-in fog-aware draw culling (video.fog_cull, default off): the farthest view
// distance that can still matter - where fog reaches 99.5% at the weakest (top of
// frame) height gradient, so the culled shell is fully fogged even before the
// stronger ground gradient. Conservative floors: never closer than 150 world units
// (the championship-final far plane precedent) and never closer than 1.25x the
// near-field clear distance, so road made readable by video.fog_clear cannot pop
// unfogged at the cull edge. Returns 0 when culling does not apply. Known caveats:
// the far-plane sky cap keeps culled-edge silhouettes 15 percent visible (a haze
// band, more prominent the closer the edge), distant trees stop occluding god-ray
// shafts near the horizon, and any saving is unmeasured - this trades draw work
// for a look change, it is not a proven performance feature.
inline float Fog_CullDistance(int amountPercent, float clearDist) {
    float d = Fog_Density(amountPercent);
    if (!(d > 0.f)) return 0.f;
    float z = 5.298317f / d;   // -ln(0.005) / density, height gradient 1.0
    float floor = clearDist * 1.25f;
    if (floor < 150.f) floor = 150.f;
    return z < floor ? floor : z;
}
// Fog depth probe verdict (video.fog_probe readback analysis, gfx_gpu.cpp): classifies one
// captured region from its median recovered view distance, the mean fog factor the curve
// predicts there and the mean before/after color shift the pass actually applied.
// 1 = near depth (residual pollution), 2 = weak density curve, 3 = strong expected factor
// but no output shift (pass/compositing failure), 4 = fog active, 5 = projection unknown.
inline int Fog_ProbeVerdict(bool haveProj, float medianViewZ, float meanExpected, float meanMeasured) {
    if (!haveProj) return 5;
    if (medianViewZ < 3.f) return 1;
    if (meanExpected < 0.15f) return 2;
    if (meanMeasured < 0.02f) return 3;
    return 4;
}
// Fallback when the projection is unknown: raw hardware depth is hyperbolic, so with a
// near 0.1 / far 150 projection depth 0.990 is about 9 units, 0.9993 about 60, 1.0 the far
// plane. Ramp over that top slice; the curve is a stand-in, not a measured distance.
inline float Fog_FactorRawDepth(float depth, float density) {
    if (!(density > 0.f)) return 0.f;
    float t = (depth - 0.990f) / 0.010f;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    return 1.f - std::exp(-density * 150.f * t * t);
}
// Ground-mist gradient on the screen v coordinate (0 top, 1 bottom): 0.55x at the top of
// the frame, 1.45x at the bottom, smoothstepped. Mirrors the shader's smoothstep exactly.
inline float Fog_HeightScale(float uvY) {
    float t = (uvY - 0.15f) / 0.70f;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    t = t * t * (3.f - 2.f * t);
    return 0.55f + 0.9f * t;
}
// The sky (far plane) hazes over but never becomes a flat wall.
inline float Fog_SkyCap(float factor, float depth) {
    if (depth < 0.9999f) return factor;
    return factor < 0.85f ? factor : 0.85f;
}
// Sun-ward glow of the fog ("light cutting through"): cubic falloff from the sun's
// projected position. dx/dy are aspect-corrected uv offsets; strength is 0..1
// (video.fog_glow / 100). At full strength the glow reaches about 0.6 screen heights.
inline float Fog_SunGlow(float dx, float dy, float strength) {
    if (!(strength > 0.f)) return 0.f;
    float d = std::sqrt(dx * dx + dy * dy);
    float g = 1.f - d * 1.6f;
    if (g < 0.f) return 0.f;
    return strength * g * g * g;
}
// Car-light glow of the fog (video.fog_lights): quadratic radial falloff from the
// light's projected position. dx/dy are aspect-corrected uv offsets; radius is the
// reach in screen heights (0.18), intensity 0..1. The shader couples the result to
// the local fog factor, so only foggy air lights up.
inline float Fog_LightGlow(float dx, float dy, float intensity, float radius) {
    if (!(intensity > 0.f) || !(radius > 0.f)) return 0.f;
    float t = 1.f - (dx * dx + dy * dy) / (radius * radius);
    if (t <= 0.f) return 0.f;
    return intensity * t * t;
}
// Car-light beam wedge (video.fog_lights): SCREEN-SPACE APPROXIMATION of the air
// lit along a beam, not a volumetric or world-space simulation - it brightens the
// already-rendered scene where the beam's screen wedge overlaps it, and it never
// changes how the road or terrain itself is lit. (dx,dy) are aspect-corrected uv
// offsets from the lamp; (dirX,dirY) the aspect-corrected unit direction the beam
// travels on screen; len the projected beam length in the same units (~0 when the
// beam points at the camera). The wedge is brightest right at the lamps and decays
// along the beam, so the air between the lamps and the horizon lights up first;
// a small radial halo keeps the lamp itself glowing when the wedge foreshortens
// away. Mirrors the light loop in shaders/fog.frag.
inline float Fog_LightBeam(float dx, float dy, float dirX, float dirY, float len, float intensity) {
    if (!(intensity > 0.f)) return 0.f;
    float wedge = 0.f;
    if (dirX * dirX + dirY * dirY > 1e-6f) {
        float a = dx * dirX + dy * dirY;          // axial distance along the beam
        float l = dx * dirY - dy * dirX;          // signed lateral offset
        float reach = len > 0.05f ? len : 0.05f;
        float width = 0.015f + (a > 0.f ? a : 0.f) * 0.35f;   // cone spread
        float lat = l / width;
        if (lat > 1.f) lat = 1.f;
        if (lat < -1.f) lat = -1.f;
        float lateral = 1.f - lat * lat;
        // Ramps in over the first 0.05 of the beam (where the halo owns the
        // glow) and is zero behind the lamp: the beam never shines backward.
        float axial = a <= 0.f ? 0.f : (a < 0.05f ? a / 0.05f : 1.f);
        wedge = axial * lateral * lateral / (1.f + 6.f * a / reach);
    }
    float halo = Fog_LightGlow(dx, dy, 1.f, 0.05f);            // foreshortened lamp
    return intensity * (wedge + halo);
}
// Soft shoulder for the accumulated light: approaches 1 per channel, never clips hard.
inline float Fog_SoftShoulder(float x) {
    if (x <= 0.f) return 0.f;
    return 1.f - std::exp(-2.f * x);
}
// Headroom: dims the light where the fogged scene is already near white, so a bright
// horizon cannot be pushed into a clipped patch while dark road still takes the beam.
inline float Fog_LightHeadroom(float r, float g, float b) {
    float m = r > g ? r : g;
    if (b > m) m = b;
    float h = (1.f - m) * 1.5f;
    if (h < 0.f) h = 0.f;
    if (h > 1.f) h = 1.f;
    return h;
}
// Fog colour: cool grey-blue away from the sun, warm inside the glow. glow is Fog_SunGlow
// clamped to 0..1 for the tint; the shader additionally brightens by 1 + glow * 0.6.
inline void Fog_Color(float glow, float *r, float *g, float *b) {
    float t = glow < 0.f ? 0.f : glow > 1.f ? 1.f : glow;
    *r = 0.62f + (0.98f - 0.62f) * t;
    *g = 0.66f + (0.87f - 0.66f) * t;
    *b = 0.72f + (0.70f - 0.72f) * t;
}
#endif
