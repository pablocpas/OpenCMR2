#ifndef OPENCMR2_SPEED_STREAK_H
#define OPENCMR2_SPEED_STREAK_H
// Speed streak: a directional radial streak (scenery stretches outward from the screen centre), not a temporal blend.
// Visual only. The maths here is mirrored by shaders/speedstreak.frag (tests/speed_streak_wiring.py checks the constants match).
// The clear inner zone is an artistic radial exclusion around the screen centre. It is NOT a car mask.
#include <math.h>
static const float kSpeedStreakInner = 0.30f;  // radius (in screen heights from the centre, aspect corrected) with no streak
static const float kSpeedStreakRamp = 0.30f;   // width of the smooth ramp from clear to full streak
static const float kSpeedStreakMaxLen = 0.10f; // longest sample line, as a fraction of the way to the centre, at full strength
static const int kSpeedStreakTaps = 12;
inline float SpeedStreak_Smoothstep(float a, float b, float x)
{
    float t = (x - a) / (b - a);
    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    return t * t * (3.0f - 2.0f * t);
}
// Streak weight 0..1 at radius r for a strength 0..1.
inline float SpeedStreak_Weight(float r, float strength)
{
    return SpeedStreak_Smoothstep(kSpeedStreakInner, kSpeedStreakInner + kSpeedStreakRamp, r) * (strength < 0.0f ? 0.0f : strength > 1.0f ? 1.0f : strength);
}
// Fraction of the pixel-to-centre vector sampled at this radius and strength.
inline float SpeedStreak_Length(float r, float strength) { return SpeedStreak_Weight(r, strength) * kSpeedStreakMaxLen; }
// Car protection: the clear inner zone is a circle around the focus, not a car mask, so
// the chase/drone car's rear sits outside it and smears (the double-shape ghost). This
// multiplier keeps the streak (and the lines) off the player car's view-depth band
// [zMin, zMax], ramping back to full across fade on both sides; the shader also
// weights each radial tap by this at the TAP's depth, so background pixels next to
// the car stop pulling car colour outward. Mirrored in shaders/speedstreak.frag
// (carProtect uniform).
inline float SpeedStreak_CarProtect(float sceneZ, float zMin, float zMax, float fade)
{
    float d = sceneZ < zMin ? zMin - sceneZ : sceneZ > zMax ? sceneZ - zMax : 0.0f;
    return SpeedStreak_Smoothstep(0.0f, fade, d);
}
// Strength from the speed ratio (may exceed 1) and the 0..100 option; the hold forces full speed (overdo test only).
inline float SpeedStreak_Strength(float ratio, int hold, int amount)
{
    if (hold) ratio = 1.0f;
    ratio = !(ratio > 0.0f) ? 0.0f : ratio > 1.0f ? 1.0f : ratio;
    int a = amount < 0 ? 0 : amount > 100 ? 100 : amount;
    return ratio * (float)a / 100.0f;
}
#endif
