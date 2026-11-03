#ifndef OPENCMR2_AIR_MOTE_H
#define OPENCMR2_AIR_MOTE_H
#include <cmath>
#include <cstring>
struct Texture;
#include "Particle.h"
#include "LightField.h"
#include "SnowGlint.h"
// Optional floating dust motes (video.air_motes, default off): pale specks drifting
// slowly in the air around the viewed car, visible at any speed including a standstill.
// This is an atmosphere effect, deliberately unlike the removed speed motes (a speed cue
// whose look the user rejected): these hang in the air and drift instead of streaming
// past. Visual only: uses the existing 400-particle pool, never touches physics, the
// collision world, the road line or the global rand stream (own LCG in the caller).
constexpr int AIR_MOTE_TYPE = 0x22;
constexpr int AIR_MOTE_BASE = 48;    // live cap at amount 100
constexpr int AIR_MOTE_LIFE = 150;   // 25 Hz steps = 6 s
constexpr int AIR_MOTE_MAX_AMOUNT = 500;
// Peak alpha: raised 130 -> 190 after the first native look found the specks too faint
// against bright fog. Second native look ("smaller and more plentiful, like small
// bugs"): size 0x3800 -> 0x2000 and the amount ceiling 300 -> 500 (cap 144 -> 240).
// Third native look ("much smaller"): size 0x2000 -> 0x1000 (see StageTiming); count,
// cap and alpha stay - the density, not the speck size, carries the effect.
constexpr int AIR_MOTE_ALPHA_MAX = 190;
// video.air_motes_amount (0..500): spawn rate and, above 100, the live cap grow with it.
inline int AirMote_Cap(int amountPercent) {
    int a = amountPercent < 0 ? 0 : amountPercent > AIR_MOTE_MAX_AMOUNT ? AIR_MOTE_MAX_AMOUNT : amountPercent;
    return AIR_MOTE_BASE * a / 100;
}
// Same pool discipline as the dust plume and speed motes: the last 64 slots stay
// original-only. The cap above the base count applies while the pool is less than
// three-quarters full (raised from half for the "more plentiful" revision: any other
// spawn may still evict a mote first (see Particle_Spawn), so a fuller mote share can
// never starve original particles; priority is unchanged).
inline bool AirMote_HasBudget(int active, int motes, int capacity, int amountPercent) {
    if (!(capacity > 64 && active < capacity - 64)) return false;
    if (motes < AIR_MOTE_BASE) return true;
    return motes < AirMote_Cap(amountPercent) && active < capacity * 3 / 4;
}
// Shared drift the motes wander around: stage wind/16 plus most of the viewed car's
// velocity, set by the caller (AirMotes_Step) so the swarm travels WITH the car - the
// third native look ("they float and fly around on their own instead of looking like
// black spots we drive into"). Set per spawn by the caller too: the wander seed the
// init callback stamps into the particle.
extern FixVector g_airMoteDrift;
extern unsigned g_airMoteNextSeed;
// Sun forward-scatter strength 0..1 for the light response below, published per
// step by AirMotes_Step (view direction vs the stage sun, weather-gated).
extern float g_airMoteSunScatter;
// Bugs, not sinking dust: p.position is the per-step velocity (16.16 fixed), eased
// snappily toward the shared drift plus a per-particle wander - a smooth loop whose
// direction, turn rate, speed and bob phase come from the spawn-time seed, so every
// mote flies its own little path. No sink: the vertical component oscillates gently.
// Alpha fades in and out as before. Pure maths, covered by tests/air_mote_spawn.cpp.
inline void AirMote_Step(Particle &p, const FixVector &drift) {
    unsigned seed;
    std::memcpy(&seed, p.pad0x5c, 4);
    if (seed == 0) seed = 0x9e3779b9u;   // particles spawned before the init callback existed stay deterministic
    float steps = (float)(p.age >> 16);
    float ang0 = (seed & 0xffff) * 6.2831853f / 65536.f;
    float turn = 0.5f + ((seed >> 16) & 0xff) * (1.5f / 255.f);   // 0.5..2.0 rad/s, per mote
    float spd = 0.3f + ((seed >> 24) & 0xff) * (0.6f / 255.f);    // 0.3..0.9 units/s, per mote
    float ang = ang0 + steps * turn / 25.f;                       // 25 Hz steps
    int wx = (int)std::lround(std::cos(ang) * spd * 65536.f / 25.f);
    int wz = (int)std::lround(std::sin(ang) * spd * 65536.f / 25.f);
    int wy = (int)std::lround(std::sin(ang0 * 2.f + steps * (0.8f + turn) / 25.f) * 0.25f * 65536.f / 25.f);
    p.position.x += (drift.x + wx - p.position.x) / 24;
    p.position.z += (drift.z + wz - p.position.z) / 24;
    p.position.y += (drift.y + wy - p.position.y) / 24;
    p.sourceVector.x += p.position.x;
    p.sourceVector.y += p.position.y;
    p.sourceVector.z += p.position.z;
    p.vector0x28 = p.sourceVector;
    int elapsed = AIR_MOTE_LIFE - (p.age >> 16);
    if (elapsed < 0) elapsed = 0;
    if (elapsed > AIR_MOTE_LIFE) elapsed = AIR_MOTE_LIFE;
    int fadeIn = elapsed < 30 ? elapsed : 30;
    int fadeOut = AIR_MOTE_LIFE - elapsed;
    if (fadeOut > 50) fadeOut = 50;
    p.type0x52 = p.type0x56;
    p.type0x56 = (BYTE)(AIR_MOTE_ALPHA_MAX * fadeIn * fadeOut / (30 * 50));
    // Light response: motes brighten toward the sun (forward scatter, with a
    // per-mote flicker from its wander seed) and inside car light beams, from
    // the same light field the fog response and snow glints use. Colour only -
    // alpha stays inside AIR_MOTE_ALPHA_MAX so the specks keep their softness.
    float m = 1.f;
    if (g_airMoteSunScatter > 0.f)
        m += g_airMoteSunScatter * (0.5f + 0.5f * std::sin(ang * 1.7f + (float)(seed & 0xff))) * 1.5f;
    for (int li = 0; li < g_fogLightSourceCount && li < LIGHT_FIELD_MAX; ++li) {
        const FogLightSource *L = &g_fogLightSources[li];
        m += SnowGlint_ConeBoost(p.vector0x28.x / 65536.f, p.vector0x28.y / 65536.f, p.vector0x28.z / 65536.f,
                                 L->pos.x / 65536.f, L->pos.y / 65536.f, L->pos.z / 65536.f,
                                 L->dir.x / 65536.f, L->dir.y / 65536.f, L->dir.z / 65536.f,
                                 L->intensity / 65536.f) * 1.5f;
    }
    if (m > 1.f) {
        if (m > 2.2f) m = 2.2f;
        int cr = (int)(0xf2 * m), cg = (int)(0xea * m), cb = (int)(0xd8 * m);
        p.colour[0] = (BYTE)(cr > 255 ? 255 : cr);
        p.colour[1] = (BYTE)(cg > 255 ? 255 : cg);
        p.colour[2] = (BYTE)(cb > 255 ? 255 : cb);
    }
}
// Spawn placement in a ring around a centre point (the viewed car): distance 3..24 world
// units, area-uniform angle and radius, 0.3..4.0 units above the ground. u1..u3 in 0..1.
// Pure maths, covered by tests/air_mote_spawn.cpp.
inline void AirMote_SpawnOffset(float u1, float u2, float u3, float *dx, float *dz, float *height) {
    float angle = 6.2831853f * u1;
    float distance = 3.f + 21.f * std::sqrt(u2 < 0.f ? 0.f : u2 > 1.f ? 1.f : u2);
    *dx = std::cos(angle) * distance;
    *dz = std::sin(angle) * distance;
    *height = 0.3f + 3.7f * (u3 < 0.f ? 0.f : u3 > 1.f ? 1.f : u3);
}
// Spawn rate in particles per 25 Hz step at amount 100: 8/s keeps the 48-strong cap
// populated against the 6 s life without a visible burst.
inline float AirMote_SpawnRate(int amountPercent) {
    int a = amountPercent < 0 ? 0 : amountPercent > AIR_MOTE_MAX_AMOUNT ? AIR_MOTE_MAX_AMOUNT : amountPercent;
    return 0.32f * (a / 100.f);
}
void AirMote_Update(Particle *, ParticleType *, SceneNode *);
void AirMote_Init(Particle *, ParticleType *, void *);
void AirMote_Spawn(FixVector *world);
void AirMotes_Step(int step16_16);
void SnowGlint_Step(int step16_16);
void AirMote_LogStats(int stepsElapsed);
void AirMote_GetStats(int *live, unsigned *drawn);
#endif

// video.air_motes_cockpit_size (percent of the normal speck, 25..100, default
// 50): the 2026-11-01 native look found the floating specks too big in the
// cockpit view - the driver sits inside the swarm, so the same world size
// reads larger. Cockpit (view type 3) only; every other view keeps the
// calibrated size exactly.
inline int AirMote_CockpitSize(int baseSize, int viewType, int sizePercent) {
    int p = sizePercent < 25 ? 25 : sizePercent > 100 ? 100 : sizePercent;
    return viewType == 3 ? baseSize * p / 100 : baseSize;
}
// video.air_motes_size (percent of the calibrated speck, 25..100, default 60): the 2026-11-04 request "make the floating specks
// smaller in every view" (after the cockpit-only one above). Applies to every camera; the cockpit still takes its own extra
// scale on top (default 60 x 50 = 30 percent there). 100 = the old calibrated size outside the cockpit.
inline int AirMote_Size(int baseSize, int viewType, int cockpitPercent, int allViewsPercent) {
    int a = allViewsPercent < 25 ? 25 : allViewsPercent > 100 ? 100 : allViewsPercent;
    return AirMote_CockpitSize(baseSize, viewType, cockpitPercent) * a / 100;
}
