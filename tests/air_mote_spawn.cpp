#include <initializer_list>
#include "port/types.h"
#include "AirMote.h"
FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
int g_fogLightSourceCount = 0;
float g_airMoteSunScatter = 0.f;
#include <cstdio>
#include <cmath>
#include <cstring>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    // Cap and budget: base 48 at amount 100, up to 240 at 500, last 64 pool slots stay original-only.
    CHECK(AirMote_CockpitSize(0x1000, 3, 50) == 0x800);   // cockpit: half size at the default
    CHECK(AirMote_CockpitSize(0x1000, 0, 50) == 0x1000);  // other views: unchanged
    CHECK(AirMote_CockpitSize(0x1000, 1, 50) == 0x1000);
    CHECK(AirMote_CockpitSize(0x1000, 5, 50) == 0x1000);
    CHECK(AirMote_CockpitSize(0x1000, 3, 100) == 0x1000); // full size opt-out
    CHECK(AirMote_CockpitSize(0x1000, 3, 0) == 0x400);    // clamped to the 25% floor
    CHECK(AirMote_CockpitSize(0x1000, 3, 999) == 0x1000); // clamped to 100%
    CHECK(AirMote_Size(0x1000, 0, 50, 60) == 0x999);      // chase/other view: 60 percent
    CHECK(AirMote_Size(0x1000, 3, 50, 60) == 0x4cc);      // cockpit: 50 percent then 60 percent
    CHECK(AirMote_Size(0x1000, 0, 50, 100) == 0x1000);    // 100 = the old size outside the cockpit
    CHECK(AirMote_Size(0x1000, 1, 50, 0) == 0x400);       // clamped to the 25% floor
    CHECK(AirMote_Cap(100) == 48);
    CHECK(AirMote_Cap(0) == 0);
    CHECK(AirMote_Cap(300) == 144);
    CHECK(AirMote_Cap(500) == 240);
    CHECK(AirMote_Cap(900) == 240);
    CHECK(!AirMote_HasBudget(0, 0, 64, 100));          // pool of 64 leaves no original reserve
    CHECK(!AirMote_HasBudget(400 - 64, 0, 400, 100));  // only 64 slots left: refuse
    CHECK(AirMote_HasBudget(100, 47, 400, 100));
    CHECK(!AirMote_HasBudget(100, 48, 400, 100));      // at the base cap
    CHECK(!AirMote_HasBudget(100, 100, 400, 100));     // above base cap needs amount > 100...
    CHECK(AirMote_HasBudget(100, 100, 400, 300));      // ...and a pool less than three-quarters full
    CHECK(AirMote_HasBudget(250, 100, 400, 300));      // three-quarter rule: still room at 250/400
    CHECK(!AirMote_HasBudget(300, 100, 400, 300));     // pool three-quarters full: no motes past base
    // Spawn ring: distance 3..24 world units, height 0.3..4.0, any angle.
    for (float u1 : {0.f, .25f, .5f, .75f, 1.f}) for (float u2 : {0.f, .5f, 1.f}) for (float u3 : {0.f, .5f, 1.f}) {
        float dx, dz, h;
        AirMote_SpawnOffset(u1, u2, u3, &dx, &dz, &h);
        float dist = std::sqrt(dx*dx + dz*dz);
        CHECK(dist >= 3.f - 1e-3f && dist <= 24.f + 1e-3f);
        CHECK(h >= 0.3f - 1e-6f && h <= 4.f + 1e-6f);
    }
    { float dx, dz, h; AirMote_SpawnOffset(0.f, 0.f, 0.f, &dx, &dz, &h); CHECK(std::fabs(dx - 3.f) < 1e-3f && std::fabs(dz) < 1e-3f && std::fabs(h - 0.3f) < 1e-6f); }
    // sqrt distribution keeps density uniform over the disc area: E[dist^2] lands mid-ring.
    double mean2 = 0; int n = 0;
    for (int i = 0; i < 2000; ++i) {
        float u2 = (i + 0.5f) / 2000.f, dx, dz, h;
        AirMote_SpawnOffset(0.31f, u2, 0.7f, &dx, &dz, &h);
        mean2 += dx*dx + dz*dz; ++n;
    }
    mean2 /= n;
    double expect = 9.0 + 126.0*(2.0/3.0) + 441.0/2.0;  // E[(3+21*sqrt(U))^2] = 9 + 126*E[sqrt U] + 441*E[U]
    CHECK(std::fabs(mean2 - expect) / expect < 0.03);
    // Rate: 0.32 per step at 100 (= 8/s at 25 Hz), scales with amount, clamps.
    CHECK(std::fabs(AirMote_SpawnRate(100) - 0.32f) < 1e-7f);
    CHECK(AirMote_SpawnRate(0) == 0.f);
    CHECK(std::fabs(AirMote_SpawnRate(300) - 0.96f) < 1e-6f);
    CHECK(std::fabs(AirMote_SpawnRate(900) - 1.6f) < 1e-6f);   // clamps at the 500 ceiling
    // Step: velocity eases toward the shared drift plus a bounded per-particle wander (bug
    // flight, no sink); alpha fades in and out, never negative, never above AIR_MOTE_ALPHA_MAX.
    for (int ageSteps : {0, 15, 30, 75, 100, 149, 150, 200}) {
        Particle p = {};
        p.age = ageSteps << 16;
        p.position = {0, 0, 0};
        p.sourceVector = {100 << 16, 50 << 16, -30 << 16};
        FixVector drift = {0x10000, 0, -0x8000};
        AirMote_Step(p, drift);
        CHECK(p.type0x56 <= AIR_MOTE_ALPHA_MAX);
        CHECK(p.vector0x28.x == p.sourceVector.x && p.vector0x28.z == p.sourceVector.z);
        // one step eases 1/24 toward drift + wander, wander bounded at 0.9 units/s
        int maxWanderStep = (int)(0.9f * 65536.f / 25.f) + 2;
        CHECK(p.position.x >= (drift.x - maxWanderStep) / 24 - 1 && p.position.x <= (drift.x + maxWanderStep) / 24 + 1);
        int maxBob = (int)(0.25f * 65536.f / 25.f) + 2;
        CHECK(p.position.y >= (-maxBob) / 24 - 1 && p.position.y <= (drift.y + maxBob) / 24 + 1);   // bob, no monotonic sink
        int alpha = p.type0x56;
        if (ageSteps == 0) CHECK(alpha == 0);                  // brand-new mote is invisible
        if (ageSteps == 150) CHECK(alpha == 0);                // dead of old age
        if (ageSteps == 75) CHECK(alpha > 0);
    }
    // Wander is per-particle and deterministic: same seed same flight, different seed different flight,
    // seed untouched by stepping, zero seed falls back to the shared constant.
    {   auto fresh = []{ Particle q = {}; q.age = 75 << 16; q.sourceVector = {100 << 16, 50 << 16, -30 << 16}; return q; };
        unsigned s1 = 0x11111111u, s2 = 0xabcdef01u, fallback = 0x9e3779b9u;
        Particle b = fresh(); std::memcpy(b.pad0x5c, &s1, 4);
        Particle bCopy = b;                                   // same seed, same age, same position
        Particle c = fresh(); std::memcpy(c.pad0x5c, &s2, 4);
        Particle zero = fresh();                              // pad zeroed
        Particle explicitFallback = fresh(); std::memcpy(explicitFallback.pad0x5c, &fallback, 4);
        FixVector drift = {0x10000, 0, -0x8000};
        for (int i = 0; i < 30; ++i) {                        // a few seconds of flight
            AirMote_Step(b, drift); AirMote_Step(bCopy, drift); AirMote_Step(c, drift);
            AirMote_Step(zero, drift); AirMote_Step(explicitFallback, drift);
        }
        CHECK(b.position.x == bCopy.position.x && b.position.y == bCopy.position.y && b.position.z == bCopy.position.z);
        CHECK(b.position.x != c.position.x || b.position.y != c.position.y || b.position.z != c.position.z);
        CHECK(zero.position.x == explicitFallback.position.x && zero.position.y == explicitFallback.position.y);
        unsigned seedAfter; std::memcpy(&seedAfter, b.pad0x5c, 4);
        CHECK(seedAfter == s1);                               // the step never touches the stamped seed
    }
    if (!fails) std::puts("air_mote_spawn ok");
    return fails;
}
