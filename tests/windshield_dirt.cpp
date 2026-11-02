// Windshield dirt map maths (game/WindshieldDirt.h): spatter locality and
// glass masking, wetness ramp, dry vs wet wipe behaviour, decay,
// determinism. Pure CPU, no renderer.
#include "WindshieldDirt.h"
#include <cstdio>
#include <cstring>

static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)

int main()
{
    // Spatter: dirt lands only inside the glass mask, localised.
    WindshieldMap m;
    Windshield_Reset(&m, 42);
    Windshield_AddSpatter(&m, 24, 200, 0.5f);
    CHECK(Windshield_TotalDirt(&m) > 0);
    for (int y = 0; y < WINDSHIELD_MAP_H; y++)
        for (int x = 0; x < WINDSHIELD_MAP_W; x++) {
            float u = (x + 0.5f) / WINDSHIELD_MAP_W, v = (y + 0.5f) / WINDSHIELD_MAP_H;
            if (!Windshield_InsideGlass(u, v))
                CHECK(m.dirt[y * WINDSHIELD_MAP_W + x] == 0);
        }
    // Determinism: same seed, same map.
    WindshieldMap m2;
    Windshield_Reset(&m2, 42);
    Windshield_AddSpatter(&m2, 24, 200, 0.5f);
    CHECK(memcmp(m.dirt, m2.dirt, sizeof m.dirt) == 0);

    // Wetness: ramps up fast, down slow, clamps.
    WindshieldMap w;
    Windshield_Reset(&w, 7);
    for (int i = 0; i < 60; i++) Windshield_UpdateWetness(&w, 255);
    CHECK(w.wet == 255);
    for (int i = 0; i < 60; i++) Windshield_UpdateWetness(&w, 0);
    CHECK(w.wet < 255 && w.wet > 0);   // slow drying

    // Dry wipe: the arc clears almost fully, faint residue, no smear deposit.
    WindshieldMap d;
    Windshield_Reset(&d, 9);
    for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++)
        d.dirt[i] = Windshield_InsideGlass((i % WINDSHIELD_MAP_W + 0.5f) / WINDSHIELD_MAP_W,
                                           (i / WINDSHIELD_MAP_W + 0.5f) / WINDSHIELD_MAP_H) ? 200 : 0;
    d.wet = 0;
    int cx = (int)(0.216f * WINDSHIELD_MAP_W), cy = (int)(0.536f * WINDSHIELD_MAP_H);   // on the blade at angle -0.25
    int before = d.dirt[cy * WINDSHIELD_MAP_W + cx];
    int removed = Windshield_Wipe(&d, 0.34f, 1.02f, 0.72f, -0.25f, 0.05f, 1);
    CHECK(before == 200 && removed > 0);
    CHECK(d.dirt[cy * WINDSHIELD_MAP_W + cx] <= 200 / 8 + 1);   // residue <= 12.5%
    // Off-arc cells untouched.
    CHECK(d.dirt[8] == 0 || Windshield_InsideGlass(0.5f, 0.1f));

    // Wet wipe: less removed, and a streak appears at the band rim.
    WindshieldMap s;
    Windshield_Reset(&s, 9);
    for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++)
        s.dirt[i] = Windshield_InsideGlass((i % WINDSHIELD_MAP_W + 0.5f) / WINDSHIELD_MAP_W,
                                           (i / WINDSHIELD_MAP_W + 0.5f) / WINDSHIELD_MAP_H) ? 200 : 0;
    s.wet = 255;
    int removedWet = Windshield_Wipe(&s, 0.34f, 1.02f, 0.72f, -0.25f, 0.05f, 1);
    CHECK(removedWet < removed);   // wet glass keeps more dirt behind
    CHECK(s.dirt[cy * WINDSHIELD_MAP_W + cx] <= 200 / 8 + 1);

    // Decay: a dirty map slowly cleans on the 64-frame sweep.
    WindshieldMap e;
    Windshield_Reset(&e, 3);
    Windshield_AddSpatter(&e, 24, 100, 0.5f);
    int t0 = Windshield_TotalDirt(&e);
    for (int f = 0; f < 64; f++) Windshield_Decay(&e, f);
    CHECK(Windshield_TotalDirt(&e) < t0);

    // Visibility regression: driving spatters at the real rate (11/s, amount 1)
    // must leave a readable map after ~20 s (old model: ~0% of glass >= 25%).
    WindshieldMap v;
    Windshield_Reset(&v, 5);
    int f = 0;
    for (int step = 0; step < 220; step++) {
        Windshield_AddSpatter(&v, 1, 111, 0.604f);
        for (int k = 0; k < 5; k++) Windshield_Decay(&v, f++ * 1);
    }
    int glass = 0, strong = 0;
    for (int y = 0; y < WINDSHIELD_MAP_H; y++)
        for (int x = 0; x < WINDSHIELD_MAP_W; x++)
            if (Windshield_InsideGlass((x + 0.5f) / WINDSHIELD_MAP_W, (y + 0.5f) / WINDSHIELD_MAP_H)) {
                glass++; if (v.dirt[y * WINDSHIELD_MAP_W + x] >= 64) strong++;
            }
    CHECK(strong * 100 >= glass * 5);

    // Broken-glass overlay suppression: the dirt overlay hides only when the
    // resolved front glass is BROKEN; intact, damaged and feature-off keep it.
    CHECK(Windshield_OverlayVisible(-1) == 1);
    CHECK(Windshield_OverlayVisible(0) == 1);
    CHECK(Windshield_OverlayVisible(1) == 1);
    CHECK(Windshield_OverlayVisible(2) == 0);

    std::printf(fails ? "FAILED\n" : "windshield dirt ok\n");
    return fails ? 1 : 0;
}
