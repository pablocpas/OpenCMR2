#ifndef OPENCMR2_GRADE_CONDITIONS_H
#define OPENCMR2_GRADE_CONDITIONS_H
// Per-condition race grade for video.color_grading: turns live stage
// signals (weather kind, sun elevation, light warmth) into one of three
// looks - clear day, golden hour, wet - blended smoothly, and always the
// exact neutral grade (0,0,100,100) when the effect strength is 0, so the
// off path stays bit-identical to the shader identity. Pure value mapping;
// the frame wiring lives at the race viewport's per-frame hook.
#include <cmath>
namespace GradeConditions {

struct Grade { int temp; int tint; int saturation; int contrast; };

static inline float Clamp01(float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }

// Smoothstep; inverted edges (edge0 > edge1) give the reversed ramp.
static inline float Smooth(float edge0, float edge1, float x)
{
    float t = Clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

// Exponential approach used for the wet blend: kind flips are discrete at
// route intervals, so the look cross-fades instead of snapping. About 63%
// there in 0.7s, over 90% in 2s, and an exact landing on a large dt.
static inline float Step(float current, float target, float dt)
{
    float f = dt / 0.7f;
    if (f > 1.0f) f = 1.0f;
    return Clamp01(current + (target - current) * f);
}

// wet: smoothed rain/snow amount 0..1 (snow is graded wet: overcast whiteout
// wants the same cool, muted treatment). elevation: sine of the sun angle
// (-1..1). warmth: (R-B)/(R+B) of the stage light colour, -1..1.
// strength: 0..100, the video.grade_amount scale.
static inline Grade Compute(float wet, float elevation, float warmth, int strength)
{
    Grade neutral = {0, 0, 100, 100};
    if (strength <= 0) return neutral;
    wet = Clamp01(wet);
    // Golden hour needs BOTH a low sun and warm light: a low sun under a
    // neutral sky is dusk, not golden.
    float g = Smooth(0.42f, 0.18f, elevation) * Smooth(0.06f, 0.22f, warmth);
    // Recipes: temp/tint offsets and saturation/contrast percentages per 100
    // strength. Day adds a firmer S-curve and saturation; golden goes warmer
    // and deeper; wet goes cool, muted, and deepest.
    const float day[4]    = {10.0f, -2.0f,   8.0f, 14.0f};
    const float golden[4] = {24.0f,  3.0f,  14.0f, 20.0f};
    const float wetR[4]   = {-10.0f, -4.0f, -6.0f, 24.0f};
    float s = (strength > 100 ? 100 : strength) / 100.0f;
    Grade out;
    float v[4];
    for (int i = 0; i < 4; ++i) {
        float dry = day[i] + (golden[i] - day[i]) * g;
        v[i] = dry + (wetR[i] - dry) * wet;
    }
    out.temp = (int)std::lround(v[0] * s);
    out.tint = (int)std::lround(v[1] * s);
    out.saturation = 100 + (int)std::lround(v[2] * s);
    out.contrast = 100 + (int)std::lround(v[3] * s);
    return out;
}

}
#endif
